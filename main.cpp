#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <algorithm>
#include <chrono>
#include <vector>
#include <commdlg.h>

#include "External/ImGui/imgui.h"
#include "External/ImGui/imgui_internal.h"
//#include "External/ImGui/imgui_extensions.h"
#include "gui.h"
#include "sounds.h"
#include "structs.h"
#include "file_helper.h"

#ifdef _DEBUG
const bool is_debug = true;
#else
const bool is_debug = false;
#endif

HWND hwnd;

const int max_note_warmup_time = 300;
const ImVec4 playAlongButtonColor = (ImVec4)ImColor(0.5f, 0.9f, 0.4f);
const float warmupTimeRatio = 1.f;

const int max_note_score = 500;

bool useVsync = true;
float soundGraph[512];

bool _wasNumLockPressedLastFrame = false;
bool _wasNumLockRevertedLastFrame = false;

Settings currentSettings;
Recording currentRecording;
ReplayState replayState;
Score currentScore;
NoteStats* currentGameNotesStats;

const char* notesList[21]{};
const char* keysList[2]{ "Major", "Minor" };
//const char* keypadKeys[12]{ "1", "2", "3", "4", "5", "6", "7", "8", "9", "Key", "0", "C" };
const char* keypadKeys[12]{ "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "C", "Key" };
const ImGuiKey intecom2VKeypad[12]{ ImGuiKey_Keypad2,ImGuiKey_NumLock, ImGuiKey_KeypadDivide, ImGuiKey_KeypadMultiply, ImGuiKey_Keypad7, ImGuiKey_Keypad8, ImGuiKey_Keypad9,ImGuiKey_Keypad4,ImGuiKey_Keypad5,ImGuiKey_Keypad6,ImGuiKey_Keypad3,ImGuiKey_Keypad1 };
//const int idx2Keypad[12]{ 0, 3, 6, 9, 1, 4, 7, 10, 2, 5, 8, 11 };
const int idx2Keypad[12]{ 1, 4, 7, 11, 2, 5, 8, 0, 3, 6, 9, 10 };

Key currentKeySignature[7];
Note currentNoteSignature[7];

int lastFrameTime = 0;

int timeDilation = 1;

/*
-------- TODO --------

+ Selectable keys
+ Saving / loading and recording
? Exporting / importing midi (Nothing will sound good when exported as midi, because of the one octave limitation) (Scrapped)
+ Add icon
- Scoring

----------------------
 */

static LARGE_INTEGER StartingTime{ 0 };

int64_t millis()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

int GetScore(int delay)
{
	delay = max_note_score - delay;
	if (delay > max_note_score - 15)
		return max_note_score;

	return delay;
}

void RecalculateKeySignature(Scale scale, Key baseKey)
{
	auto start = std::chrono::high_resolution_clock::now();
	Sounds::GetKeySignature(scale, baseKey, currentKeySignature);
	printf("Calculating the signature for key: %s, and scale %i took: %lldus\n", Sounds::GetKeyName(baseKey), scale,
		std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count());

	//std::sort(currentKeySignature, currentKeySignature + 7);

	for (int i = 0; i < 7; i++)
		currentNoteSignature[i] = Sounds::Key2Note(currentKeySignature[i]);
}

void SetNumLock(BOOL bState)
{
	BYTE keyState[256];

	if (!GetKeyboardState((LPBYTE)&keyState))
		return;
	if ((bState && !(keyState[VK_NUMLOCK] & 1)) ||
		(!bState && (keyState[VK_NUMLOCK] & 1)))
	{
		// Simulate a key press
		keybd_event(VK_NUMLOCK,
			0x45,
			KEYEVENTF_EXTENDEDKEY | 0,
			0);

		// Simulate a key release
		keybd_event(VK_NUMLOCK,
			0x45,
			KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,
			0);
	}
}

bool SaveCurrentRecording()
{
	char filename[MAX_PATH]{ "name" };
	const char szExt[] = "IMR\0*.imr\0\0"; // Intercom Music Recording (IMR)

	bool wasSucc = false;

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = ofn.lpstrDefExt = szExt;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_OVERWRITEPROMPT;

	if (GetSaveFileName(&ofn))
	{
		strcpy_s(currentRecording.savePath, filename);

		wasSucc = FileHelper::SaveRecording(currentRecording, filename);
	}

	return wasSucc;
}

bool OpenRecording()
{
	char filename[MAX_PATH]{ "name" };
	const char szExt[] = "IMR\0*.imr\0\0";

	bool wasSucc = false;

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = ofn.lpstrDefExt = szExt;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_OVERWRITEPROMPT;

	if (GetOpenFileName(&ofn))
	{
		wasSucc = FileHelper::OpenRecording(currentRecording, filename);

		strcpy_s(currentRecording.savePath, filename);
	}

	return wasSucc;
}

int OnGui()
{
	ImGuiIO& io = ImGui::GetIO();
	ImGuiStyle& style = ImGui::GetStyle();

#ifdef _DEBUG
	ImGui::Text("Average framerate: %f", io.Framerate);
#endif // _DEBUG

	bool wasNewRecEntryAdded = false;

	if (ImGui::BeginChild("Keypad", { 270,360 }, true))
	{
		auto keypadAvail = ImGui::GetContentRegionAvail();
		if (ImGui::BeginTable("KeypadTable", 3))
		{
			ImGui::TableNextRow();

			keypadAvail.x -= style.CellPadding.x * 2 + style.WindowPadding.x;
			keypadAvail.y -= style.CellPadding.y * 3 + style.WindowPadding.y;
			ImGui::TableSetColumnIndex(0);

			ImGui::PushStyleColor(ImGuiCol_Text, { 0,0,0,0.95f });

			Note playAlongNote = (Note)-1;
			static Note lastPlayAlongNote{ (Note)-1 };
			//static int lastNoteDelayTime = -1;
			//static int64_t lastNotePlayTime{ 0 };
			float playAlongProgress = 0;
			//static int delaysNum{ 1 };
			//static float delayTime = 0;
			bool playedCurrentNote = false;
			static int noteIdx = 0;
			static int notesCount = 0;
			static int hangingDelay {0};

			static int64_t awaitStart = 0;

			static bool wasPlayingAlongLastFrame = false;

			if (replayState.isPlayingAlong)
			{
				auto timeNow = millis() / timeDilation;

				if (!wasPlayingAlongLastFrame)
				{
					noteIdx = 0;
					awaitStart = 0;
					hangingDelay = 0;

					notesCount = std::count_if(currentRecording.data.begin(), currentRecording.data.end(), [](RecordingEvent ev) { return ev.type == RecEv_Key; });
					currentGameNotesStats = new NoteStats[notesCount]; // Remember to delete[] the array after playing
					ZeroMemory(&currentScore, sizeof(currentScore));

					int delaySum = 1000;
					int delaysNum = 0;

					for (size_t i = 0; i < currentRecording.data.size(); i++)
					{
						if (currentRecording.data[i].type == RecEv_Delay)
						{
							delaySum += currentRecording.data[i].value;
							delaysNum++;
						}
						else
						{
							currentGameNotesStats[noteIdx].delayTime = (delaySum * 100.f / replayState.playbackSpeedPercent);
							currentGameNotesStats[noteIdx].delayNum = delaysNum;

							auto warmupTime = (float)min((delaySum * warmupTimeRatio), max_note_warmup_time);
							currentGameNotesStats[noteIdx].warmupTime = warmupTime;
							currentGameNotesStats[noteIdx].actualWarmupRatio = warmupTime / delaySum;
							currentGameNotesStats[noteIdx].key = (Key)currentRecording.data[i].value;

							currentGameNotesStats[noteIdx].playTime = noteIdx == 0 ? timeNow : (currentGameNotesStats[noteIdx-1].playTime + (int64_t)currentGameNotesStats[noteIdx - 1].delayTime);

							delaySum = delaysNum = 0;

							noteIdx++;
						}
					}
					noteIdx = 0;
				}

				NoteStats& currentStats = currentGameNotesStats[noteIdx];

				float overallDelayProgress = max(min((timeNow - currentStats.playTime) / currentStats.delayTime, 1), 0);
				if (overallDelayProgress >= (1 - currentStats.actualWarmupRatio))
				{
					playAlongProgress = overallDelayProgress - (1 - currentStats.actualWarmupRatio);
					playAlongProgress /= currentStats.actualWarmupRatio;
				}
				else
					playAlongProgress = 0;

				if (timeNow - currentStats.playTime > currentStats.delayNum - currentStats.warmupTime || replayState.waitForInput && hangingDelay <= 0)
				{
					// Next event doesnt have to be a Note...
					playAlongNote = ((replayState.progress < currentRecording.data.size()-1) ? Sounds::Key2Note((Key)currentRecording.data[replayState.progress + currentStats.delayNum].value) : (Note)-1);
				}
				else
					playAlongNote = (Note)-1;

				if (((playAlongProgress >= 0.9999f && !replayState.waitForInput) || (currentStats.wasClicked && replayState.waitForInput)) && (hangingDelay <= 0 && replayState.progress < currentRecording.data.size()))
				{
					if (currentRecording.data[replayState.progress].type == RecEv_Key)
					{
						noteIdx++;
						if (noteIdx < notesCount)
						{
							printf("Set playtime for note %i\n", noteIdx);
							//currentGameNotesStats[noteIdx].playTime = timeNow;
						}
						else if (replayState.progress < currentRecording.data.size() && std::count_if(currentRecording.data.begin() + replayState.progress + 1, currentRecording.data.end(), [](RecordingEvent ev) { return ev.type == RecEv_Key; }) <= 0)
						{
							noteIdx--;
							for (size_t i = replayState.progress+1; i < currentRecording.data.size(); i++)
							{
								hangingDelay += currentRecording.data[i].value;
								replayState.progress++;
							}
							awaitStart = millis();
							//printf("hanging delay: %i\n", hangingDelay);
						}
					}
					replayState.progress += currentStats.delayNum;
					//if ((currentRecording.data[replayState.progress].type == RecEv_Key && replayState.progress == 0))
					//{
					//	replayState.progress++;
					//	replayState.lastNotePlayTime = timeNow;
					//}
					replayState.wasInputNotePressed = false;
				}

				//switch (currentRecording.data[replayState.progress].type)
				//{
				//case RecEv_Key:
				//	if (replayState.progress > 0 || (replayState.progress == 0 && playAlongProgress >= 0.9999f))
				//	{
				//		lastPlayAlongNote = Sounds::Key2Note((Key)currentRecording.data[replayState.progress].value);
				//		playAlongNote = Sounds::Key2Note((Key)currentRecording.data[replayState.progress].value);
				//		replayState.lastNotePlayTime = timeNow;
				//		replayState.progress++;
				//		playedCurrentNote = false;
				//		break;
				//	}
				//	// Fallthrough on porpouse.
				//case RecEv_Delay:
				//	// This part can only run when progress changes
				//	if (replayState.progress == currentRecording.data.size() - 1 && currentRecording.data[replayState.progress].type == RecEv_Delay)
				//	{
				//		replayState.progress++;
				//		break;
				//	}
				//	if (currentRecording.data[replayState.progress].type == RecEv_Key && replayState.progress == 0)
				//		delaysNum = -1;
				//	else
				//		delaysNum = 1;
				//	delayTime = (float)(replayState.progress == 0) ? 1000 : currentRecording.data[replayState.progress].value;
				//	if (currentRecording.data[replayState.progress + 1].type == RecEv_Delay)
				//	{
				//		for (size_t i = replayState.progress + 1; i < currentRecording.data.size(); i++)
				//		{
				//			if (currentRecording.data[i].type == RecEv_Delay)
				//				delayTime += currentRecording.data[i].value;
				//			else
				//				break;
				//			delaysNum++;
				//		}
				//	}
				//	delayTime = (delayTime * 100 / replayState.playbackSpeedPercent);
				//	auto warmupTime = (float)min((delayTime * warmupTimeRatio), max_note_warmup_time);
				//	float overallDelayProgress = max(min((timeNow - replayState.lastNotePlayTime) / delayTime, 1), 0);
				//	//playAlongProgress *= (playAlongProgress > (1 - warmupTimeRatio)) ? ;
				//	auto currentRatio = warmupTime / delayTime;
				//	if (overallDelayProgress >= (1 - currentRatio))
				//	{
				//		playAlongProgress = overallDelayProgress - (1 - currentRatio);
				//		playAlongProgress /= currentRatio;
				//	}
				//	else
				//		playAlongProgress = 0;
				//	if (timeNow - replayState.lastNotePlayTime > delayTime - warmupTime || replayState.waitForInput)
				//	{
				//		// Next event doesnt have to be a Note...
				//		playAlongNote = ((replayState.progress < currentRecording.data.size()) ? Sounds::Key2Note((Key)currentRecording.data[replayState.progress + delaysNum].value) : (Note)-1);
				//	}
				//	else
				//		playAlongNote = (Note)-1;
				//	//printf("warmuptime is: %f. time left %f\n", warmupTime, playAlongProgress);
				//	if ((playAlongProgress >= 0.9999f && !replayState.waitForInput) || (replayState.wasInputNotePressed && replayState.waitForInput))
				//	{
				//		replayState.progress += delaysNum;
				//		if ((currentRecording.data[replayState.progress].type == RecEv_Key && replayState.progress == 0))
				//		{
				//			replayState.progress++;
				//			replayState.lastNotePlayTime = timeNow;
				//		}
				//		replayState.wasInputNotePressed = false;
				//	}
				//	break;
				//}

				//if ((timeNow - currentStats.playTime) - currentStats.delayTime - hangingDelay > 0)
				//	hangingDelay = 0;

				if (replayState.progress >= currentRecording.data.size() && (((timeNow - currentStats.playTime) - currentStats.delayTime) > (max_note_score + hangingDelay)))
				{
					currentScore.accuracy = (float)currentScore.points / (notesCount * max_note_score);
					printf("Score: %lld, Misses: %i, Acc: %f%%\n", currentScore.points, currentScore.badNotes, currentScore.accuracy * 100);

					for (int i = 0; i < notesCount; i++)
					{
						if(!currentGameNotesStats[i].wasClicked)
							currentScore.missedNotes++;
					}

					replayState.lastNotePlayTime = -1;
					replayState.progress = 0;
					replayState.isPlayingAlong = false;
					noteIdx = 0;
					awaitStart = 0;
					hangingDelay = 0;
					delete[] currentGameNotesStats;

					if(!replayState.waitForInput)
					ImGui::OpenPopup("Score##ScoreEndPopup");
				}

				wasPlayingAlongLastFrame = true;

				/*
				if (replayState.lastNotePlayTime < 0)
					replayState.lastNotePlayTime = timeNow;
				else
				{
					if (replayState.progress < 0)
						replayState.progress = 0;
					//if (replayState.progress == 0 && currentRecording.data[0].type == RecEv_Delay)
					//	replayState.lastNotePlayTime = timeNow + 1000;

					switch (currentRecording.data[replayState.progress].type)
					{
					case RecEv_Key:
						if (replayState.progress > 0 || (replayState.progress == 0 && playAlongProgress >= 0.9999f))
						{
							lastPlayAlongNote = Sounds::Key2Note((Key)currentRecording.data[replayState.progress].value);
							lastNoteDelayTime = delayTime;
							lastNotePlayTime = replayState.lastNotePlayTime;
							playAlongNote = Sounds::Key2Note((Key)currentRecording.data[replayState.progress].value);
							replayState.lastNotePlayTime = timeNow;
							replayState.progress++;
							playedCurrentNote = false;
							break;
						}
						// Fallthrough on porpouse.
					case RecEv_Delay:
						// This part can only run when progress changes
						if (replayState.progress == currentRecording.data.size() - 1 && currentRecording.data[replayState.progress].type == RecEv_Delay)
						{
							replayState.progress++;
							break;
						}
						if (currentRecording.data[replayState.progress].type == RecEv_Key && replayState.progress == 0)
							delaysNum = -1;
						else
							delaysNum = 1;
						delayTime = (float)(replayState.progress == 0) ? 1000 : currentRecording.data[replayState.progress].value;
						if (currentRecording.data[replayState.progress + 1].type == RecEv_Delay)
						{
							for (size_t i = replayState.progress + 1; i < currentRecording.data.size(); i++)
							{
								if (currentRecording.data[i].type == RecEv_Delay)
									delayTime += currentRecording.data[i].value;
								else
									break;
								delaysNum++;
							}
						}
						delayTime = (delayTime * 100 / replayState.playbackSpeedPercent);
						auto warmupTime = (float)min((delayTime * warmupTimeRatio), max_note_warmup_time);
						float overallDelayProgress = max(min((timeNow - replayState.lastNotePlayTime) / delayTime, 1), 0);
						//playAlongProgress *= (playAlongProgress > (1 - warmupTimeRatio)) ? ;
						auto currentRatio = warmupTime / delayTime;
						if (overallDelayProgress >= (1 - currentRatio))
						{
							playAlongProgress = overallDelayProgress - (1 - currentRatio);
							playAlongProgress /= currentRatio;
						}
						else
							playAlongProgress = 0;
						if (timeNow - replayState.lastNotePlayTime > delayTime - warmupTime || replayState.waitForInput)
						{
							// Next event doesnt have to be a Note...
							playAlongNote = ((replayState.progress < currentRecording.data.size()) ? Sounds::Key2Note((Key)currentRecording.data[replayState.progress + delaysNum].value) : (Note)-1);
						}
						else
							playAlongNote = (Note)-1;
						//printf("warmuptime is: %f. time left %f\n", warmupTime, playAlongProgress);
						if ((playAlongProgress >= 0.9999f && !replayState.waitForInput) || (replayState.wasInputNotePressed && replayState.waitForInput))
						{
							replayState.progress += delaysNum;
							if ((currentRecording.data[replayState.progress].type == RecEv_Key && replayState.progress == 0))
							{
								replayState.progress++;
								replayState.lastNotePlayTime = timeNow;
							}
							replayState.wasInputNotePressed = false;
						}
						break;
					}

					if (replayState.progress > currentRecording.data.size() - 1)
					{
						int notesNum = 0;
						for (size_t i = 0; i < replayState.progress; i++)
						{
							if (currentRecording.data[i].type == RecEv_Key)
								notesNum++;
						}
						currentScore.accuracy = (float)currentScore.points / (notesNum * max_note_score);
						printf("Score: %lld, Misses: %i, Acc: %f%%\n", currentScore.points, currentScore.badNotes, currentScore.accuracy * 100);
						ZeroMemory(&currentScore, sizeof(currentScore));

						replayState.lastNotePlayTime = -1;
						replayState.progress = 0;
						replayState.isPlayingAlong = false;

						delete[] currentGameNotesStats;
					}
				}
				*/
			}
			else
				wasPlayingAlongLastFrame = false;

				if (ImGui::BeginPopupModal("Score##ScoreEndPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
				{
					const int width = 260;
					if (ImGui::BeginTable("ScoreTable", 3, ImGuiTableFlags_SizingStretchSame))
					{
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Score: %i", currentScore.points);
						ImGui::TableNextColumn();
						ImGui::Text("Notes hit: %i/%i", notesCount - currentScore.missedNotes, notesCount);
						ImGui::TableNextColumn();
						ImGui::Text("Bad notes: %i", currentScore.badNotes);
						ImGui::EndTable();
					}
					ImGui::Text("Accuracy: %.2f%%", currentScore.accuracy * 100);

					if (ImGui::Button("Close", { width, 0 }))
					{
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}

			for (int i = 0; i <= 11; i++)
			{
				int keypadIdx = idx2Keypad[i];

				auto currentKey = Sounds::Note2Key((Note)keypadIdx);

				bool isEnabled = !currentSettings.useScale || (std::find(currentNoteSignature, currentNoteSignature + 7, keypadIdx) != currentNoteSignature + 7);

				bool isPlayAlongNote = playAlongNote == (Note)keypadIdx;
				bool isSoonToBePlayedAlong = false;

				if (isEnabled)
					for (int x = 0; x < 7; x++)
					{
						Note note = Sounds::Key2Note(currentKeySignature[x]);
						if (note == (Note)keypadIdx)
						{
							currentKey = currentKeySignature[x];
							break;
						}
					}

				auto timeNow = millis() / timeDilation;

				const char* keyName = Sounds::GetKeyName((isEnabled && currentSettings.useScale) ? currentKey : Sounds::Note2Key((Note)keypadIdx));
				char spaceBuffer[5]{ '\0' };
				if (keypadIdx != 11)
					memset(spaceBuffer, ' ', min(strlen(keyName) + 1, 5));

				char buffer[64]{ '\0' };
				sprintf_s(buffer, "%s%s\n(%s)###KeyPad%i", spaceBuffer, keypadKeys[keypadIdx], keyName, keypadIdx);

				size_t latestNote = 0;

				static long long haltIdx = -1;

				if (replayState.isPlayingAlong)
				{
					float noteProgress = 0;

					for (size_t i = 0; i < notesCount; i++)
					{
						NoteStats& curNote = currentGameNotesStats[i];
						//auto timePlayed = curNote.playTime;//(i == 0 ? curNote.playTime : curNote.playTime > 0 ? curNote.playTime : (currentGameNotesStats[i - 1].playTime + (int64_t)currentGameNotesStats[i - 1].delayTime));
						//auto timeDelta = abs(timeNow - curNote.playTime - curNote.delayTime);

						if (Sounds::Key2Note(curNote.key) == (Note)keypadIdx && replayState.waitForInput && haltIdx > -1)
						{
							curNote.playTime += lastFrameTime;
							if (haltIdx == i)
								curNote.playTime = timeNow - (int64_t)curNote.delayTime;
						}

						auto timeLeft = timeNow - curNote.playTime - curNote.delayTime;

						//if (abs(timeLeft) > 100 && haltIdx > -1)
						//	auto x = 0;

						if (abs(timeLeft) < max_note_warmup_time && Sounds::Key2Note(curNote.key) == (Note)keypadIdx && !curNote.wasClicked)
						{
							//auto points = GetScore(timeDelta);

							//if (abs(timeLeft) < 50)
							//	noteProgress = 1;
							//else
							if(noteProgress == 0)
							noteProgress = min(max(timeLeft / max_note_warmup_time, -1), 1) + 1;
							if (replayState.waitForInput && noteProgress >= 1 && haltIdx < 0)
							{
								noteProgress = 1;
								haltIdx = i;
							}
							if (noteProgress <= 1)
								isSoonToBePlayedAlong = true;

							//printf("Note %s (%lld) time left: %f, progress: %f\n", keyName, i, timeNow - curNote.playTime - curNote.delayTime, noteProgress);
						}

						if (i == haltIdx && haltIdx > -1 && curNote.wasClicked)
						{
							haltIdx = -1;
						}
					}

					if (isSoonToBePlayedAlong)
					{
						ImGui::PushStyleColor(ImGuiCol_Button, ImLerp(style.Colors[ImGuiCol_Button], (ImVec4)ImColor::HSV(.2f, 1.f, 1.f), noteProgress * 1.1f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImLerp(style.Colors[ImGuiCol_ButtonHovered], (ImVec4)ImColor::HSV(.2f, 1.f, 0.92f), noteProgress * 1.1f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImLerp(style.Colors[ImGuiCol_ButtonActive], (ImVec4)ImColor::HSV(.2f, 1.f, 0.85f), noteProgress * 1.1f));
					}
					//ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(.2f, playAlongProgress, 0.85f));
					//ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(.2f, playAlongProgress, 0.92f));
					//ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(.2f, playAlongProgress, 1.f));
				}
				else
					haltIdx = -1;

				if (!isEnabled)
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, style.Alpha * style.DisabledAlpha);

				bool button = ImGui::Button(buffer, { keypadAvail.x / 3, keypadAvail.y / 4 }) || ImGui::IsKeyPressed(intecom2VKeypad[keypadIdx], false);

				if (button || (replayState.isPlayingAlong && (isPlayAlongNote) && !replayState.waitForInput && playAlongProgress >= 0.9999f))
				{
					if ( ( (button && !replayState.isPlayingAlong) || (replayState.isPlayingAlong && !currentGameNotesStats[noteIdx].wasPlayed) ) )
					{
						if (i == 0)
						{
							if (!_wasNumLockRevertedLastFrame)
								Sounds::PlaySound((Note)keypadIdx);
						}
						else
							Sounds::PlaySound((Note)keypadIdx);

						if(replayState.isPlayingAlong)
							currentGameNotesStats[noteIdx].wasPlayed = true;
					}


					if (replayState.isPlayingAlong)
					{
						if (isPlayAlongNote)
						{
							replayState.wasInputNotePressed = true;
							playedCurrentNote = true;
							currentGameNotesStats[noteIdx].wasPlayed = true;
						}

						if ((button && i != 0) || (button && i == 0 && !_wasNumLockRevertedLastFrame))
						{
							printf("pressed %s (%i)\n", keyName, noteIdx);
							bool foundNote = false;
							//if (noteIdx == 7)
							//	auto x = 0;
							for (size_t i = 0; i < notesCount; i++)
							{
								NoteStats& curNote = currentGameNotesStats[i];
								auto timeDelta = abs(timeNow - curNote.playTime - curNote.delayTime);
								if (!curNote.wasClicked && timeDelta < max_note_score && Sounds::Key2Note(curNote.key) == (Note)keypadIdx)
								{
									auto points = GetScore(timeDelta);
									printf("Note %zu delay %f, points awarded: %i\n", i, timeDelta, points);
									curNote.wasClicked = true;
									curNote.wasPlayed = true;

									currentScore.points += points;
									foundNote = true;
									break;
								}
							}
							if (!foundNote)
								currentScore.badNotes++;
						}
					}
					else
					{
						if (currentSettings.isRecording && !currentSettings.isRecordingPaused)
						{
							auto notePlayTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
							if (currentRecording.data.size() > 0)
								currentRecording.data.push_back({ RecEv_Delay, currentSettings.useStaticDelay ? currentSettings.delay : (int)(notePlayTime - currentRecording.lastKeyTime) });
							else
								currentRecording.data.push_back({ RecEv_Delay, 0 });

							currentRecording.lastKeyTime = notePlayTime;

							currentRecording.data.push_back({ RecEv_Key, (isEnabled && currentSettings.useScale) ? currentKey : Sounds::Note2Key((Note)keypadIdx) });

							wasNewRecEntryAdded = true;
						}
						else if (!currentSettings.isRecordingPaused)
						{
							if (currentRecording.data.size() > 0 && currentRecording.selectedEvent >= 0)
							{
								currentRecording.data[currentRecording.selectedEvent].value = currentKey;
							}
						}
						if (replayState.isPlayingAlong)
						{
							currentScore.badNotes++;
						}
					}
				}
				if (!isEnabled)
					ImGui::PopStyleVar();

				if (isSoonToBePlayedAlong)
					ImGui::PopStyleColor(3);

				if ((i + 1) % 4 == 0 && i < 10)
					ImGui::TableSetColumnIndex((i) / 4 + 1);
			}

			//if(ImGui::Button("   1\n(A#)###KeyPad1", { keypadAvail.x/3,keypadAvail.y/4 }) || (ImGui::IsKeyPressed(ImGuiKey_NumLock, false) && !_wasNumLockRevertedLastFrame))
			//{
			//	Sounds::PlaySound(As);
			//	//memcpy(soundGraph, Sounds::bufferMem + Sounds::sampleRate - 256, 256*sizeof(float));
			//	//Sleep(220);
			//	//Sounds::PlaySound(Gs);
			//	//memcpy(soundGraph + 256, Sounds::bufferMem, 256 * sizeof(float));
			//}
			//
			//if (ImGui::Button("   4\n(C#)###KeyPad4", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_Keypad7, false))
			//{
			//	Sounds::PlaySound(Cs);
			//}
			//
			//if (ImGui::Button("  7\n(E)###KeyPad7", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_Keypad4, false))
			//{
			//	Sounds::PlaySound(E);
			//}
			//
			//if (ImGui::Button("Key\n (G)###KeyPad10", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_Keypad1, false))
			//{
			//	Sounds::PlaySound(G);
			//}
			//ImGui::TableSetColumnIndex(1);
			//
			//if (ImGui::Button("  2\n(B)###KeyPad2", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_KeypadDivide, false))
			//{
			//	Sounds::PlaySound(B);
			//}
			//
			//if (ImGui::Button("  5\n(D)###KeyPad5", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_Keypad8, false))
			//{
			//	Sounds::PlaySound(D);
			//}
			//
			//if (ImGui::Button("  8\n(F)###KeyPad8", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_Keypad5, false))
			//{
			//	Sounds::PlaySound(F);
			//}
			//
			//if (ImGui::Button("  0\n(A)###KeyPad11", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_Keypad2, false))
			//{
			//	Sounds::PlaySound(A);
			//}
			//ImGui::TableSetColumnIndex(2);
			//
			//if (ImGui::Button("  3\n(C)###KeyPad3", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_KeypadMultiply, false))
			//{
			//	Sounds::PlaySound(C);
			//}
			//
			//if (ImGui::Button("   6\n(D#)###KeyPad6", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_Keypad9, false))
			//{
			//	Sounds::PlaySound(Ds);
			//}
			//
			//if (ImGui::Button("   9\n(F#)###KeyPad9", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_Keypad6, false))
			//{
			//	Sounds::PlaySound(Fs);
			//}
			//
			//if (ImGui::Button("   C\n(G#)###KeyPad12", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_Keypad3, false))
			//{
			//	Sounds::PlaySound(Gs);
			//}
			ImGui::PopStyleColor();

			ImGui::EndTable();
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
	ImGui::BeginGroup();
	{
		auto settingsAvail = ImGui::GetContentRegionAvail();

		ImGui::HeaderTitle("Sound");

		ImVec2 scaleAvail = { settingsAvail.x - style.ItemSpacing.x * 2 , settingsAvail.y };
		ImGui::PushItemWidth((settingsAvail.x - style.ItemSpacing.x) / 2 - ImGui::GetFrameHeight());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x, 0 });
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, style.FramePadding.y });

		ImGui::BeginDisabled(!currentSettings.useScale);
		ImGui::Dummy({ ImGui::GetFrameHeight() , 0 }); ImGui::SameLine();
		ImGui::LabelText("###NoteLabel", "Note"); ImGui::SameLine(); ImGui::LabelText("###KeyLabel", "Key");
		ImGui::EndDisabled();
		ImGui::PopStyleVar();

		ImGui::Checkbox("###ScaleCB", &currentSettings.useScale); ImGui::SameLine();
		ImGui::BeginDisabled(!currentSettings.useScale);
		if (ImGui::Combo("###NoteCombo", (int*)&currentSettings.selectedKey, notesList, 21))
			RecalculateKeySignature(currentSettings.selectedScale, currentSettings.selectedKey);
		ImGui::SameLine();
		ImGui::PopStyleVar();
		if (ImGui::Combo("###KeyCombo", (int*)&currentSettings.selectedScale, keysList, 2))
			RecalculateKeySignature(currentSettings.selectedScale, currentSettings.selectedKey);
		ImGui::EndDisabled();

		ImGui::PopItemWidth();


		ImGui::Text("Octave");
		ImGui::SliderInt("###OctaveSlider", &currentSettings.octave, 0, 10);
		//ImGui::Dummy({ 0, style.ItemSpacing.x });

		ImGui::Text("Volume");
		ImGui::SliderFloat("###VolumeSlider", &currentSettings.volume, 0, 1);
		//ImGui::Dummy({ 0, style.ItemSpacing.x });

		//ImGui::Text("Use Saw Wave");
		//ImGui::Checkbox("###Use Saw WaveCB", &currentSettings.useSawWave);
		ImGui::Spacing();



		// Recording
		ImGui::HeaderTitle("Recording");

		ImGui::BeginGroup();
		ImGui::BeginDisabled(replayState.isPlaying || replayState.isPlayingAlong);

		ImGui::Checkbox("Static Delay", &currentSettings.useStaticDelay);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::BeginDisabled(!currentSettings.useStaticDelay);
		ImGui::DragInt("###DelayDrag", &currentSettings.delay, 1, 1, 1000, "%dms");
		ImGui::EndDisabled();

		if (ImGui::Button(currentSettings.isRecording ? "Stop" : "Record", { (settingsAvail.x - style.ItemSpacing.x) / 2, 0 }))
		{
			if (!currentSettings.isRecording && currentRecording.data.size() > 0)
				ImGui::OpenPopup("Clear previeus?##ClearPrevieusRecordingPopup");
			else
			{
				currentSettings.isRecording = !currentSettings.isRecording;
				currentSettings.isRecordingPaused = false;
			}
		}

		if (ImGui::BeginPopupModal("Clear previeus?##ClearPrevieusRecordingPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Do you want to clear the previeus\nrecording before starting a new one?");
			ImGui::Separator();

			auto avail = ImGui::GetContentRegionAvail().x - style.ItemSpacing.x * 2;

			if (ImGui::Button("Clear", { avail / 3, 0 }))
			{
				currentRecording.data.clear();
				ImGui::CloseCurrentPopup();

				currentSettings.isRecording = !currentSettings.isRecording;
				currentSettings.isRecordingPaused = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("Record", { avail / 3, 0 }))
			{
				ImGui::CloseCurrentPopup();

				currentSettings.isRecording = !currentSettings.isRecording;
				currentSettings.isRecordingPaused = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", { avail / 3, 0 }))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::SameLine();

		ImGui::BeginDisabled(!currentSettings.isRecording);
		if (ImGui::Button(currentSettings.isRecordingPaused ? "Resume" : "Pause", { (settingsAvail.x - style.ItemSpacing.x) / 2, 0 }))
		{
			currentSettings.isRecordingPaused = !currentSettings.isRecordingPaused;
		}
		ImGui::EndDisabled();

		ImGui::BeginDisabled(currentRecording.data.size() <= 0);
		if (ImGui::Button("Save", { (settingsAvail.x - style.ItemSpacing.x) / 2, 0 }))
		{
			SaveCurrentRecording();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(currentSettings.isRecording);
		if (ImGui::Button("Open", { (settingsAvail.x - style.ItemSpacing.x) / 2, 0 }))
		{
			OpenRecording();
		}
		ImGui::EndDisabled();
		if (ImGui::Button("Clear", { settingsAvail.x , 0 }))
		{
			ImGui::OpenPopup("Are you sure?##ClearRecordingPopup");
		}

		if (ImGui::BeginPopupModal("Are you sure?##ClearRecordingPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Are you sure you want to clear the recording?");
			ImGui::Separator();

			auto avail = ImGui::GetContentRegionAvail().x - style.ItemSpacing.x;

			if (ImGui::Button("Clear", { avail / 2, 0 }))
			{
				currentRecording.data.clear();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", { avail / 2, 0 }))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		ImGui::EndDisabled();
		ImGui::EndGroup();
		if (ImGui::IsItemHovered() && (replayState.isPlaying || replayState.isPlayingAlong))
			ImGui::SetTooltip("Stop the replay first");
	}
	ImGui::EndGroup();

	ImGui::PopItemWidth();

	if (ImGui::BeginChild("Replay", { 0, is_debug ? 180 : -1 }, true))
	{
		ImGui::BeginDisabled(currentSettings.isRecording);
		ImVec2 replayAvail = ImGui::GetContentRegionAvail();

		float controlButtonWidth = (replayAvail.x - style.ItemSpacing.x) / 2;

		ImGui::BeginDisabled(currentRecording.data.size() <= 0 || replayState.isPlayingAlong);

		bool wasStopClicked = false;

		if (ImGui::Button(replayState.isPlaying ? "Stop##PlayReplay" : "Play##PlayReplay", { controlButtonWidth, 0 }))
		{
			replayState.isPlaying = !replayState.isPlaying;
			if (replayState.isPlaying)
				Sounds::PlayReplay(currentRecording, &replayState.isPlaying, &replayState.isPaused, &replayState.progress, &replayState.playbackSpeedPercent);
			else
				replayState.progress = 0;

			replayState.isPaused = false;
			wasStopClicked = true;
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(!replayState.isPlaying);
		if (ImGui::Button(replayState.isPaused ? "Resume##PlayReplay" : "Pause##PlayReplay", { controlButtonWidth, 0 }))
		{
			replayState.isPaused = !replayState.isPaused;
			if (!replayState.isPaused)
				Sounds::PlayReplay(currentRecording, &replayState.isPlaying, &replayState.isPaused, &replayState.progress, &replayState.playbackSpeedPercent);
		}
		ImGui::EndDisabled();

		static bool wasLastItemVisible = false;
		static bool wasLastTimePlaying = false;

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.9f, 0.9f, 0.9f, 1.00f));
		if (ImGui::BeginChild("ReplayNodes", { replayAvail.x, ImGui::GetFrameHeight() + style.ScrollbarSize }, false, ImGuiWindowFlags_AlwaysHorizontalScrollbar))
		{
			//ImGui::Dummy({ 5, ImGui::GetFrameHeight() });
			//ImGui::SameLine();
			auto _currentRecCopy = currentRecording;
			for (size_t i = 0; i < currentRecording.data.size(); i++)
			{
				//bool isSelected = currentRecording.selectedEvent == i;
				//if(ImGui::RecEvent(currentRecording.data[i], selected == i, i, {0, 0}))
				//	selected = i;
				char popupName[36];
				sprintf_s(popupName, "Delete this item?##ItemDel%zu", i);
				bool is_selected = _currentRecCopy.selectedEvent == i;
				auto item = _currentRecCopy.data[i];

				switch (_currentRecCopy.data[i].type)
				{
				case RecEv_Key:
					//if (i == 0)
					//{
					//	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (style.FramePadding.y/2));
					//}
					if (ImGui::DrawRecEvKey(_currentRecCopy.data[i], is_selected, i))
						currentRecording.selectedEvent = i;

					if (is_selected && ImGui::IsMouseReleased(0) && ImGui::IsItemHovered())
					{
						currentRecording.selectedEvent = -1;
					}

					// Scroll Only when playing
					if (((replayState.isPlaying && !replayState.isPaused || replayState.isPlayingAlong)) && replayState.progress == (i + replayState.isPlaying) && replayState.autoScroll)
					{
						//ImGui::ScrollToItem();
						ImGui::SetScrollHereX(0);
					}

					// Set progress when scrolling (only when not playing)
					if (replayState.autoScroll && (!(replayState.isPlaying && !replayState.isPaused) && !replayState.isPlayingAlong) && !wasLastItemVisible && ImGui::IsItemVisible())
					{
						replayState.progress = i;
					}

					wasLastItemVisible = ImGui::IsItemVisible();
					break;
				case RecEv_Delay:
					ImGui::DrawRecEvDelay(currentRecording.data[i], i, _currentRecCopy.moveMode);
					if (ImGui::IsItemClicked(0))
						currentRecording.selectedEvent = i;
					break;
				default:
					break;
				}

				if (wasStopClicked && i == 0)
					ImGui::SetScrollHereX(0);

				if (wasLastTimePlaying && (!replayState.isPlaying && !replayState.isPlayingAlong) && i == 0)
					ImGui::SetScrollHereX(0);

				if (ImGui::IsItemActive() && !ImGui::IsItemHovered() && currentRecording.moveMode)
				{
					printf("Curr Delta: %f\n", ImGui::GetMouseDragDelta(0).x);
					int n_next = i + (ImGui::GetMouseDragDelta(0).x < 0.f ? -1 : 1);
					printf("Next item: %i\n", n_next);
					if (n_next >= 0 && n_next < currentRecording.data.size() && ImGui::GetMouseDragDelta(0).x != 0)
					{
						currentRecording.data[i] = currentRecording.data[n_next];
						currentRecording.data[n_next] = item;
						ImGui::ResetMouseDragDelta();
						printf("\nMoved!\n\n");
					}
				}

				if (ImGui::IsItemClicked(1))
				{
					ImGui::OpenPopup(popupName);
				}

				if (ImGui::BeginPopup(popupName))
				{
					if (ImGui::Button("Delete"))
					{
						currentRecording.data.erase(currentRecording.data.begin() + i);
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}

				if (i < currentRecording.data.size() - 1)
					ImGui::SameLine();
			}
			ImGui::SameLine();
			ImGui::Dummy({ -8, 10 });
			if (currentSettings.isRecording && !currentSettings.isRecordingPaused && wasNewRecEntryAdded && replayState.autoScroll)
				ImGui::SetScrollHereX(0);

			wasLastTimePlaying = (replayState.isPlaying && !replayState.isPaused) || replayState.isPlayingAlong;
		}
		ImGui::PopStyleColor();
		ImGui::EndChild();

		ImGui::Checkbox("Move Mode", &currentRecording.moveMode);
		ImGui::SameLine();
		if (ImGui::Checkbox("Auto Scroll", &replayState.autoScroll) && (!(replayState.isPlaying && !replayState.isPaused) && !replayState.isPlayingAlong))
			replayState.progress = 0;
		ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize("Insert Delay").x - ImGui::CalcTextSize("Insert Note").x - style.FramePadding.x * 4 - style.WindowPadding.x - style.ItemSpacing.x);
		ImGui::BeginDisabled(currentRecording.data.size() <= 0);
		ImGui::BeginGroup();
		if (ImGui::Button("Insert Delay", { 0,0 }))
		{
			currentRecording.data.insert(currentRecording.data.begin() + ((currentRecording.selectedEvent >= 0) ? currentRecording.selectedEvent : currentRecording.data.size() - 1) + 1, { RecEv_Delay , currentSettings.useStaticDelay ? currentSettings.delay : 200 });
		}
		ImGui::SameLine();
		if (ImGui::Button("Insert Note", { 0,0 }))
		{
			currentRecording.data.insert(currentRecording.data.begin() + ((currentRecording.selectedEvent >= 0) ? currentRecording.selectedEvent : currentRecording.data.size() - 1) + 1, { RecEv_Key , currentSettings.useScale ? currentKeySignature[0] : 0 });
		}
		ImGui::EndGroup();
		ImGui::EndDisabled();


		ImGui::HeaderTitle("Play Along");

		ImGui::BeginDisabled(currentRecording.data.size() <= 0);
		ImGui::BeginDisabled(replayState.isPlaying);
		if (ImGui::Button(replayState.isPlayingAlong ? "Stop##PlayAlong" : "Play##PlayAlong", { controlButtonWidth, 0 }))
		{
			replayState.isPlayingAlong = !replayState.isPlayingAlong;
			replayState.wasInputNotePressed = false;
			replayState.lastNotePlayTime = -1;
			replayState.progress = 0;
			ZeroMemory(&currentScore, sizeof(currentScore));
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::SetNextItemWidth(controlButtonWidth - ImGui::CalcTextSize("Speed").x);
		ImGui::SliderInt("Speed", &replayState.playbackSpeedPercent, 1, 300, "%d%%");

		ImGui::BeginDisabled(replayState.isPlayingAlong);
		if (ImGui::Checkbox("Practice Mode", &replayState.waitForInput))
		{
			if (replayState.waitForInput && replayState.isPlayingAlong)
				replayState.progress = max(replayState.progress - 2, 0);
		}
		ImGui::EndDisabled();

		ImGui::EndDisabled();
		ImGui::EndDisabled();
	}
	ImGui::EndChild();
	if (ImGui::IsItemHovered() && currentSettings.isRecording)
		ImGui::SetTooltip("Stop the recording first");

#ifdef _DEBUG
	ImGui::PlotLines("AudioFunc", Sounds::bufferMem, 512 / (*Sounds::pBaseOctave + 1), 0, 0, 1.f, -1.f, { ImGui::GetContentRegionAvail().x, -1 });
#endif

	if (_wasNumLockPressedLastFrame && GetForegroundWindow() == hwnd)
	{
		_wasNumLockRevertedLastFrame = true;
		SetNumLock(true);
	}
	else
		_wasNumLockRevertedLastFrame = false;

	_wasNumLockPressedLastFrame = ImGui::IsKeyDown(ImGuiKey_NumLock);

	return 0;
}

bool shouldExit = false;

bool OnExit()
{
	shouldExit = true;
	return true;
}

int main(int argc, char** argv)
{
	hwnd = GUI::Setup(OnGui);
	GUI::VSyncFrame = (UINT*)&useVsync;
	GUI::LoadFonts();
	GUI::onExitFunc = OnExit;

	Sounds::Setup();
	Sounds::pBaseOctave = &currentSettings.octave;
	Sounds::pVolume = &currentSettings.volume;
	Sounds::pUseSawWave = &currentSettings.useSawWave;

	RecalculateKeySignature(Major, Key_C);

	for (int i = 0; i < 21; i++)
	{
		notesList[i] = Sounds::GetKeyName((Key)i);
	}

	while (true)
	{
		if (shouldExit)
			break;

		auto frameStart = millis();
		GUI::DrawGui();
		lastFrameTime = millis() - frameStart;
	}

	GUI::Destroy();
	return 0;
}