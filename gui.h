#pragma once

#include <d3d11.h>
#include <vector>

namespace GUI
{
	HWND Setup(int (*OnGuiFunc)());
	int DrawGui() noexcept;
	void Destroy() noexcept;
	void LoadFonts(float fontSizeMultiplier = 1.f);

	inline IDXGISwapChain* g_pSwapChain = NULL;
	inline ID3D11Device* g_pd3dDevice = NULL;
	inline std::vector<IDXGIOutput*> vOutputs;
	inline std::vector<IDXGIAdapter*> vAdapters;
	inline bool (*onExitFunc)();

	inline UINT(*VSyncFrame);
}

#define TOOLTIP(...)					\
   	if (ImGui::IsItemHovered())			\
		ImGui::SetTooltip(__VA_ARGS__); \