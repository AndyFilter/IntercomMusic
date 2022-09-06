#include "imgui_internal.h"
#include "imgui.h"
#include "imgui_extensions.h"
#include "../sounds.h"

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

    bool DrawRecEvKey(RecordingEvent& recEv, bool is_selected, int index, ImVec2 arg_size)
    {
        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;

        char text[20]{ '\0' };
        auto key_name = Sounds::GetKeyName((Key)recEv.value);
        sprintf_s(text, "Key %s ##Key%i", key_name, index);

        auto labelSize = ImGui::CalcTextSize(text, NULL, true);

        ImVec2 size = arg_size;
        size.x = arg_size.x == 0 ? labelSize.x : arg_size.x;
        size.y = arg_size.y == 0 ? labelSize.y + style.FramePadding.y * 2.0f - style.ItemSpacing.y : arg_size.y;

        ImGui::PushStyleColor(ImGuiCol_Text, { 0,0,0,0.95f });

        ImVec4 baseKeyColor = ImVec4(0.7f, 1.f, 0.7f, 1.f);
        PushStyleColor(ImGuiCol_Header, baseKeyColor);
        baseKeyColor *= 0.85f;
        baseKeyColor.w = 1;
        PushStyleColor(ImGuiCol_HeaderHovered, baseKeyColor);
        baseKeyColor *= 0.85f;
        baseKeyColor.w = 1;
        PushStyleColor(ImGuiCol_HeaderActive, baseKeyColor);

        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { 0.5f, 0.5f });
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - style.FramePadding.y + style.ItemSpacing.y/2);
        //ImGui::Dummy({ style.ItemSpacing.x, 5 });
        //ImGui::SameLine();
        auto returnVal = Selectable(text, is_selected, ImGuiSelectableFlags_UseSpacing , size, true);
        //ImGui::SameLine();
        //ImGui::Dummy({ style.ItemSpacing.x, 5 });
        ImGui::PopStyleVar();

        ImGui::PopStyleColor(4);

        return returnVal;
    }

    bool DrawRecEvDelay(RecordingEvent& recEv, int index, bool moveMode, ImVec2 arg_size)
    {
        char text[20]{ '\0' };
        auto key_name = Sounds::GetKeyName((Key)recEv.value);
        sprintf_s(text, "##Delay%i", index);

        auto size = ImGui::CalcTextSize("1000ms", NULL, true);
        SetNextItemWidth(size.x + 5);

        ImGui::PushStyleColor(ImGuiCol_Text, { 0,0,0,0.95f });

        ImVec4 baseDelayColor = ImVec4(1.f, 0.6f, 0.6f, 1.f);
        PushStyleColor(ImGuiCol_FrameBg, baseDelayColor);
        baseDelayColor *= 0.85f;
        baseDelayColor.w = 1;
        PushStyleColor(ImGuiCol_FrameBgHovered, baseDelayColor);
        baseDelayColor *= 0.85f;
        baseDelayColor.w = 1;
        PushStyleColor(ImGuiCol_FrameBgActive, baseDelayColor);

        auto returnVal = ImGui::DragInt(text, &recEv.value, 1, 1, 1000, "%dms", moveMode ? ImGuiSliderFlags_ReadOnly : 0);

        ImGui::PopStyleColor(4);

        return returnVal;
    }

    //bool RecEvent(RecordingEvent& recEv, bool is_selected, int index, ImVec2 arg_size)
    //{
    //    switch (recEv.type)
    //    {
    //    case RecEv_Key:
    //        return DrawRecEvKey(recEv, is_selected, index, arg_size);
    //        break;
    //    case RecEv_Delay:
    //        return DrawRecEvDelay(recEv, is_selected, index, arg_size);
    //        break;
    //    default:
    //        break;
    //    }
    //    return false;
    //}
}