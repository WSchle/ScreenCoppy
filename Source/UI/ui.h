#pragma once

#include <memory>
#include <d3d11.h>

#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_impl_win32.h"
#include "../../ImGui/imgui_impl_dx11.h"

#include "../Sampler/sampler.h"

class ui
{
	HWND					 hWnd;
	float					 dpiScale[2] = { 0.0f, 0.0f };
	int						 screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int						 screenHeight = GetSystemMetrics(SM_CYSCREEN);
	ImVec4					 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	WNDCLASSEXW				 wc;
	ID3D11Device*			 pd3dDevice = nullptr;
	ID3D11DeviceContext* 	 pd3dDeviceContext = nullptr;
	IDXGISwapChain*			 pSwapChain = nullptr;
	bool                     SwapChainOccluded = false;
	
	ID3D11RenderTargetView*  mainRenderTargetView = nullptr;
	PixelSampler* pixelSampler = nullptr;

	bool CreateDeviceD3D(HWND hWnd);
	void CleanupDeviceD3D();
	void CreateRenderTarget();
	void CleanupRenderTarget();
	
	bool isSelecting = false;
	ImVec2 dragStart = { -1.f,-1.f };
	ImVec2 dragEnd = { -1.f,-1.f };

	void handleMouse();
	
	void drawDottedRect(ImVec2 topLeft, ImVec2 bottomRight);
public:
	

	ui();
	~ui();

	void drawSelection();
	void resetWindow();
};

extern std::unique_ptr<ui> UI;