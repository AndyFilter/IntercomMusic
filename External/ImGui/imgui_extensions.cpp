#include "imgui_internal.h"
#include "imgui.h"
#include "imgui_extensions.h"

namespace ImGui
{
	void HeaderTitle(const char* text, float space)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGuiContext& g = *GImGui;

		auto textSize = CalcTextSize(text);
        ImVec2 text_pos(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);

        auto lineGap = (textSize.x + space)/2;

        // Horizontal Separator
        float x1 = window->Pos.x;
        float x2 = window->Pos.x + window->Size.x;

        if (g.GroupStack.Size > 0 && g.GroupStack.back().WindowID == window->ID)
            x1 += window->DC.Indent.x;

        float lineWidth = (x2 - x1);
        x2 = lineWidth / 2 + x1 - lineGap;

        //const ImRect bb(ImVec2(x1, window->DC.CursorPos.y), ImVec2(x2, window->DC.CursorPos.y + 1));
        const ImRect bb(text_pos, text_pos + ImVec2(((window->Pos.x + window->Size.x)-x1), textSize.y));
        ItemSize(ImVec2(0.0f, 0));

		BeginGroup();
			
        const bool item_visible = ItemAdd(bb, 0);
        if (item_visible)
        {
            window->DrawList->AddLine({ bb.Min.x, bb.Min.y - (bb.Min.y - bb.Max.y)/2 }, ImVec2(x2, bb.Min.y - (bb.Min.y - bb.Max.y) / 2), GetColorU32(ImGuiCol_Separator));

            RenderTextClipped(bb.Min, bb.Max, text, NULL, NULL, { 0.5f, 0.5f }, &bb);

            window->DrawList->AddLine({ bb.Min.x + lineWidth/2 + lineGap - 2, bb.Min.y - (bb.Min.y - bb.Max.y) / 2 }, ImVec2(window->Pos.x + window->Size.x, bb.Min.y - (bb.Min.y - bb.Max.y) / 2), GetColorU32(ImGuiCol_Separator));

            Dummy({textSize.x, textSize.y - 2});
        }

		EndGroup();
	}
}