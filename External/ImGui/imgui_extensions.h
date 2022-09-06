#pragma once

#include "../structs.h"

namespace ImGui
{
	IMGUI_API void HeaderTitle(const char* text, float space = 10);

	//IMGUI_API bool RecEvent(RecordingEvent& recEv, bool is_selected, int index, ImVec2 arg_size = {0, 0});

	IMGUI_API bool DrawRecEvKey(RecordingEvent& recEv, bool is_selected, int index, ImVec2 arg_size = { 0, 0 });
	IMGUI_API bool DrawRecEvDelay(RecordingEvent& recEv, int index, bool moveMode, ImVec2 arg_size = { 0, 0 });
}