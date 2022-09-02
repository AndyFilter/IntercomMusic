#define WIN32_LEAN_AND_MEAN

#include "External/ImGui/imgui.h"
#include "gui.h"
#include "sounds.h"
#include "External/ImGui/imgui_internal.h"

HWND hwnd;

bool useVsync = true;
float soundGraph[512];

bool _wasNumLockPressedLastFrame = false;
bool _wasNumLockRevertedLastFrame = false;

Settings currentSettings;

/* 
-------- TODO --------

- Selectable keys
- Saving / loading and recording
- Exporting / importing midi
- Add icon

----------------------
 */

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
			if(ImGui::Button("   1\n(A#)###KeyPad1", { keypadAvail.x/3,keypadAvail.y/4 }) || (ImGui::IsKeyPressed(ImGuiKey_NumLock, false) && !_wasNumLockRevertedLastFrame))
			{
				Sounds::PlaySound(As);
				//memcpy(soundGraph, Sounds::bufferMem + Sounds::sampleRate - 256, 256*sizeof(float));
				//Sleep(220);
				//Sounds::PlaySound(Gs);
				//memcpy(soundGraph + 256, Sounds::bufferMem, 256 * sizeof(float));
			}

			if (ImGui::Button("   4\n(C#)###KeyPad4", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_Keypad7, false))
			{
				Sounds::PlaySound(Cs);
			}

			if (ImGui::Button("  7\n(E)###KeyPad7", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_Keypad4, false))
			{
				Sounds::PlaySound(E);
			}

			if (ImGui::Button("Key\n (G)###KeyPad10", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_Keypad1, false))
			{
				Sounds::PlaySound(G);
			}
			ImGui::TableSetColumnIndex(1);

			if (ImGui::Button("  2\n(B)###KeyPad2", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_KeypadDivide, false))
			{
				Sounds::PlaySound(B);
			}

			if (ImGui::Button("  5\n(D)###KeyPad5", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_Keypad8, false))
			{
				Sounds::PlaySound(D);
			}

			if (ImGui::Button("  8\n(F)###KeyPad8", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_Keypad5, false))
			{
				Sounds::PlaySound(F);
			}

			if (ImGui::Button("  0\n(A)###KeyPad11", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_Keypad2, false))
			{
				Sounds::PlaySound(A);
			}
			ImGui::TableSetColumnIndex(2);

			if (ImGui::Button("  3\n(C)###KeyPad3", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_KeypadMultiply, false))
			{
				Sounds::PlaySound(C);
			}

			if (ImGui::Button("   6\n(D#)###KeyPad6", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_Keypad9, false))
			{
				Sounds::PlaySound(Ds);
			}

			if (ImGui::Button("   9\n(F#)###KeyPad9", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_Keypad6, false))
			{
				Sounds::PlaySound(Fs);
			}

			if (ImGui::Button("   C\n(G#)###KeyPad12", { keypadAvail.x / 3,keypadAvail.y / 4 }) || ImGui::IsKeyPressed(ImGuiKey_Keypad3, false))
			{
				Sounds::PlaySound(Gs);
			}
			ImGui::PopStyleColor();

			ImGui::EndTable();
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
	ImGui::BeginGroup();

	ImGui::Text("Octave");
	ImGui::SliderInt("###OctaveSlider", &currentSettings.octave, 0, 10);
	//ImGui::Dummy({ 0, style.ItemSpacing.x });

	ImGui::Text("Volume");
	ImGui::SliderFloat("###VolumeSlider", &currentSettings.volume, 0, 1);
	//ImGui::Dummy({ 0, style.ItemSpacing.x });

	ImGui::Text("Use Saw Wave");
	ImGui::Checkbox("###Use Saw WaveCB", &currentSettings.useSawWave);

	ImGui::EndGroup();
	ImGui::PopItemWidth();

	ImGui::PlotLines("AudioFunc", Sounds::bufferMem, 512 / (*Sounds::pBaseOctave + 1), 0, 0, 1.f, -1.f, {ImGui::GetContentRegionAvail().x, 200});

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

	while (true)
	{
		GUI::DrawGui();
	}

	GUI::Destroy();
}