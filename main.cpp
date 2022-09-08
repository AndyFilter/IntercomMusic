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

HWND hwnd;

bool useVsync = true;
float soundGraph[512];

bool _wasNumLockPressedLastFrame = false;
bool _wasNumLockRevertedLastFrame = false;

Settings currentSettings;
Recording currentRecording;
ReplayState replayState;

const char* notesList[21]{};
const char* keysList[2] {"Major", "Minor"};
//const char* keypadKeys[12]{ "1", "2", "3", "4", "5", "6", "7", "8", "9", "Key", "0", "C" };
const char* keypadKeys[12]{ "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "C", "Key" };
const ImGuiKey intecom2VKeypad[12]{ ImGuiKey_Keypad2,ImGuiKey_NumLock, ImGuiKey_KeypadDivide, ImGuiKey_KeypadMultiply, ImGuiKey_Keypad7, ImGuiKey_Keypad8, ImGuiKey_Keypad9,ImGuiKey_Keypad4,ImGuiKey_Keypad5,ImGuiKey_Keypad6,ImGuiKey_Keypad3,ImGuiKey_Keypad1 };
//const int idx2Keypad[12]{ 0, 3, 6, 9, 1, 4, 7, 10, 2, 5, 8, 11 };
const int idx2Keypad[12]{ 1, 4, 7, 11, 2, 5, 8, 0, 3, 6, 9, 10 };

Key currentKeySignature[7];
Note currentNoteSignature[7];

/* 
-------- TODO --------

+ Selectable keys
- Saving / loading and recording
- Exporting / importing midi
- Add icon

----------------------
 */

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


	if (ImGui::BeginChild("Keypad", {270,360}, true))
	{
		auto keypadAvail = ImGui::GetContentRegionAvail();
		if (ImGui::BeginTable("KeypadTable", 3))
		{
			ImGui::TableNextRow();
			
			keypadAvail.x -= style.CellPadding.x * 2 + style.WindowPadding.x;
			keypadAvail.y -= style.CellPadding.y * 3 + style.WindowPadding.y;
			ImGui::TableSetColumnIndex(0);

			ImGui::PushStyleColor(ImGuiCol_Text, { 0,0,0,0.95f });

			for (int i = 0; i <= 11; i++)
			{
				int keypadIdx = idx2Keypad[i];

				auto currentKey = Sounds::Note2Key((Note)keypadIdx);

				bool isEnabled = !currentSettings.useScale || (std::find(currentNoteSignature, currentNoteSignature + 7, keypadIdx) != currentNoteSignature + 7);

				if(isEnabled)
					for (int x = 0; x < 7; x++)
					{
						Note note = Sounds::Key2Note(currentKeySignature[x]);
						if (note == (Note)keypadIdx)
						{
							currentKey = currentKeySignature[x];
							break;
						}
					}

				const char* keyName = Sounds::GetKeyName((isEnabled && currentSettings.useScale) ? currentKey : Sounds::Note2Key((Note)keypadIdx));
				char spaceBuffer[5]{'\0'};
				if (keypadIdx != 11)
				memset(spaceBuffer, ' ', min(strlen(keyName)+1, 5));
				char buffer[64]{ '\0'};
				sprintf_s(buffer, "%s%s\n(%s)###KeyPad%i", spaceBuffer, keypadKeys[keypadIdx], keyName, keypadIdx);


				if(!isEnabled)
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, style.Alpha * style.DisabledAlpha);
				if (ImGui::Button(buffer, { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(intecom2VKeypad[keypadIdx], false))
				{
					if (i == 0)
					{
						if (!_wasNumLockRevertedLastFrame)
							Sounds::PlaySound((Note)keypadIdx);
					}
					else
						Sounds::PlaySound((Note)keypadIdx);

					if (currentSettings.isRecording && !currentSettings.isRecordingPaused)
					{
						auto notePlayTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
						if (currentRecording.data.size() > 0)
							currentRecording.data.push_back({ RecEv_Delay, currentSettings.useStaticDelay ? currentSettings.delay : (int)(notePlayTime - currentRecording.lastKeyTime)});
						else
							currentRecording.data.push_back({ RecEv_Delay, 0 });

						currentRecording.lastKeyTime = notePlayTime;

						currentRecording.data.push_back({ RecEv_Key, (isEnabled && currentSettings.useScale) ? currentKey : Sounds::Note2Key((Note)keypadIdx) });
					}
					else if(!currentSettings.isRecordingPaused)
					{
						if (currentRecording.data.size() > 0 && currentRecording.selectedEvent >= 0)
						{
							currentRecording.data[currentRecording.selectedEvent].value = currentKey;
						}
					}
				}
				if (!isEnabled)
					ImGui::PopStyleVar();

				if ((i+1) % 4 == 0 && i < 10)
					//ImGui::Text("Space");
					ImGui::TableSetColumnIndex((i)/4+1);
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
		ImGui::PushItemWidth((settingsAvail.x - style.ItemSpacing.x)/2 - ImGui::GetFrameHeight());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x, 0 });
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, style.FramePadding.y });

		ImGui::BeginDisabled(!currentSettings.useScale);
		ImGui::Dummy({ ImGui::GetFrameHeight() , 0 }); ImGui::SameLine();
		ImGui::LabelText("###NoteLabel","Note"); ImGui::SameLine(); ImGui::LabelText("###KeyLabel","Key");
		ImGui::EndDisabled();
		ImGui::PopStyleVar();

		ImGui::Checkbox("###ScaleCB", &currentSettings.useScale); ImGui::SameLine();
		ImGui::BeginDisabled(!currentSettings.useScale);
		if(ImGui::Combo("###NoteCombo", (int*)&currentSettings.selectedKey, notesList, 21))
			RecalculateKeySignature(currentSettings.selectedScale, currentSettings.selectedKey);
		ImGui::SameLine();
		ImGui::PopStyleVar();
		if(ImGui::Combo("###KeyCombo", (int*)&currentSettings.selectedScale, keysList, 2))
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

		ImGui::Checkbox("Static Delay", &currentSettings.useStaticDelay);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::BeginDisabled(!currentSettings.useStaticDelay);
		ImGui::DragInt("###DelayDrag", &currentSettings.delay, 1, 1, 1000, "%dms");
		ImGui::EndDisabled();

		if (ImGui::Button(currentSettings.isRecording ? "Stop" : "Record", {(settingsAvail.x - style.ItemSpacing.x)/2, 0}))
		{
			currentSettings.isRecording = !currentSettings.isRecording;
			currentSettings.isRecordingPaused = false;
		}

		ImGui::SameLine();

		ImGui::BeginDisabled(!currentSettings.isRecording);
		if (ImGui::Button(currentSettings.isRecordingPaused ? "Resume" : "Pause", { (settingsAvail.x - style.ItemSpacing.x) / 2, 0 }))
		{
			currentSettings.isRecordingPaused = !currentSettings.isRecordingPaused;
		}
		ImGui::EndDisabled();

		ImGui::BeginDisabled(currentRecording.data.size() <= 0);
		if (ImGui::Button("Save", { (settingsAvail.x - style.ItemSpacing.x) / 2, 0}))
		{
			SaveCurrentRecording();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Open", { (settingsAvail.x - style.ItemSpacing.x) / 2, 0 }))
		{
			OpenRecording();
		}
		if (ImGui::Button("Clear", { settingsAvail.x , 0}))
		{
			ImGui::OpenPopup("Are you sure?##ClearRecordingPopup");
		}

		if (ImGui::BeginPopupModal("Are you sure?##ClearRecordingPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Are you sure you want to clear the recording?");
			ImGui::Separator();

			auto avail = ImGui::GetContentRegionAvail().x - style.ItemSpacing.x;

			if (ImGui::Button("Clear", { avail / 2, 0}))
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
	}
	ImGui::EndGroup();
	ImGui::PopItemWidth();

	if (ImGui::BeginChild("Replay", { 0, 140 }, true))
	{
		ImVec2 replayAvail = ImGui::GetContentRegionAvail();

		float controlButtonWidth = (replayAvail.x - style.ItemSpacing.x) / 2;

		ImGui::BeginDisabled(currentRecording.data.size() <= 0);

		if (ImGui::Button(replayState.isPlaying ? "Stop" : "Play", { controlButtonWidth, 0 }))
		{
			replayState.isPlaying = !replayState.isPlaying;
			if (replayState.isPlaying)
				Sounds::PlayReplay(currentRecording, &replayState.isPlaying, &replayState.isPaused, &replayState.progress);
			else
				replayState.progress = 0;

			replayState.isPaused = false;
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(!replayState.isPlaying);
		if (ImGui::Button(replayState.isPaused ? "Resume" : "Pause", { controlButtonWidth, 0 }))
		{
			replayState.isPaused = !replayState.isPaused;
			if (!replayState.isPaused)
				Sounds::PlayReplay(currentRecording, &replayState.isPlaying, &replayState.isPaused, &replayState.progress);
		}
		ImGui::EndDisabled();

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.9f, 0.9f, 0.9f, 1.00f));
		if (ImGui::BeginChild("ReplayNodes", { replayAvail.x, ImGui::GetFrameHeight() + style.ScrollbarSize}, false, ImGuiWindowFlags_AlwaysHorizontalScrollbar))
		{
			//ImGui::Dummy({ 5, ImGui::GetFrameHeight() });
			//ImGui::SameLine();
			auto _currentRecCopy = currentRecording;
			for (size_t i = 0; i < currentRecording.data.size(); i++)
			{
				//bool isSelected = currentRecording.selectedEvent == i;
				//if(ImGui::RecEvent(currentRecording.data[i], selected == i, i, {0, 0}))
				//	selected = i;
				char popupName[34];
				sprintf_s(popupName, "Delete this item?##ItemDel%lld", i);
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
					break;
				case RecEv_Delay:
					ImGui::DrawRecEvDelay(currentRecording.data[i], i, _currentRecCopy.moveMode);
					if (ImGui::IsItemClicked(0))
						currentRecording.selectedEvent = i;
					break;
				default:
					break;
				}

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
			ImGui::Dummy({ 0,10 });
		}
		ImGui::PopStyleColor();
		ImGui::EndChild();

		ImGui::Checkbox("Move Mode", &currentRecording.moveMode);
		ImGui::SameLine();
		if (ImGui::Button("Insert Delay", { 0,0 }))
		{
			currentRecording.data.insert(currentRecording.data.begin() + ((currentRecording.selectedEvent >= 0) ? currentRecording.selectedEvent : 0) + 1, { RecEv_Delay , 200});
		}
	}
	ImGui::EndChild();

	ImGui::PlotLines("AudioFunc", Sounds::bufferMem, 512 / (*Sounds::pBaseOctave + 1), 0, 0, 1.f, -1.f, {ImGui::GetContentRegionAvail().x, -1 });

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

int main(int argc, char** argv)
{
	hwnd = GUI::Setup(OnGui);
	GUI::VSyncFrame = (UINT*)&useVsync;
	GUI::LoadFonts();

	Sounds::Setup();
	Sounds::pBaseOctave = &currentSettings.octave;
	Sounds::pVolume = &currentSettings.volume;
	Sounds::pUseSawWave = &currentSettings.useSawWave;

	for (int i = 0; i < 21; i++)
	{
		notesList[i] = Sounds::GetKeyName((Key)i);
	}

	RecalculateKeySignature(Major, Key_C);

	while (true)
	{
		GUI::DrawGui();
	}

	GUI::Destroy();
}