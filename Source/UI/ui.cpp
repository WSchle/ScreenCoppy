#include "ui.h"

#include <string>
#include <vector>
#include <dxgi.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "../OCR/ocr.h"

const char* ws = " \t\n\r\f\v";

// trim from end of string (right)
inline std::string& rtrim(std::string& s, const char* t = ws)
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from beginning of string (left)
inline std::string& ltrim(std::string& s, const char* t = ws)
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from both ends of string (right then left)
inline std::string& trim(std::string& s, const char* t = ws)
{
    return ltrim(rtrim(s, t), t);
}

std::vector<unsigned char> CaptureScreenRect(int x, int y, int width, int height)
{
    UI->resetWindow();
    

    HDC screenDC = GetDC(NULL);
    HDC memDC = CreateCompatibleDC(screenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(screenDC, width, height);
    HGDIOBJ oldBitmap = SelectObject(memDC, hBitmap);

    BitBlt(memDC, 0, 0, width, height, screenDC, x, y, SRCCOPY);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;    // RGBA
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<unsigned char> pixels(width * height * 4);
    GetDIBits(memDC, hBitmap, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);

    // Cleanup
    SelectObject(memDC, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(NULL, screenDC);

    return pixels;
}

bool setClipboard(const std::string& text)
{
    if (!OpenClipboard(nullptr))
        return false;

    EmptyClipboard();

    size_t size = text.size() + 1; // include null terminator
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hGlobal) {
        CloseClipboard();
        return false;
    }

    char* pData = static_cast<char*>(GlobalLock(hGlobal));
    if (!pData) {
        GlobalFree(hGlobal);
        CloseClipboard();
        return false;
    }

    memcpy(pData, text.c_str(), size); // includes null terminator
    GlobalUnlock(hGlobal);

    SetClipboardData(CF_TEXT, hGlobal); // ownership transferred to clipboard
    CloseClipboard();
    return true;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
UINT                     ResizeWidth = 0, ResizeHeight = 0;

ui::ui()
{
	ImGui_ImplWin32_EnableDpiAwareness();
	dpiScale[0] = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0,0 }, MONITOR_DEFAULTTOPRIMARY));

	wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr,  nullptr,  nullptr, L"Test", nullptr };
	::RegisterClassEx(&wc);

	hWnd = ::CreateWindowExW(WS_EX_OVERLAPPEDWINDOW | WS_EX_TOPMOST | WS_EX_TRANSPARENT, wc.lpszClassName, L"ScreenCoppyOverlay", WS_MAXIMIZE | WS_POPUP, vScreenX, vScreenY, vScreenWidth, vScreenHeight, NULL, NULL, wc.hInstance, NULL);

    if (!CreateDeviceD3D(hWnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return;
    }

    ImGui_ImplWin32_EnableAlphaCompositing(hWnd);
    SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(pd3dDevice, pd3dDeviceContext);

	pixelSampler = new PixelSampler();

    RegisterHotKey(hWnd, 1, MOD_CONTROL | MOD_SHIFT, 'S');
    RegisterHotKey(hWnd, 2, 0, VK_ESCAPE);
}

void ui::handleMouse()
{
    ImGuiIO& io = ImGui::GetIO();

    POINT pt;
    GetCursorPos(&pt);
    

    if (ImGui::IsMouseClicked(0) && !ImGui::GetIO().WantCaptureMouse)
    {
        isSelecting = true;
        dragStart = ImVec2((float)pt.x, (float)pt.y);
        dragEnd = ImVec2((float)pt.x, (float)pt.y);
    }
    else if (ImGui::IsMouseReleased(0))
    {
        isSelecting = false;

        // Todo add logic to save screenshot and parse text
        int x = (int)min(dragStart.x, dragEnd.x);
        int y = (int)min(dragStart.y, dragEnd.y);
        int w = (int)abs(dragEnd.x - dragStart.x);
        int h = (int)abs(dragEnd.y - dragStart.y);

        if (w < 25 && h < 25)
        {
            resetWindow();
            return;
        }

        std::vector<unsigned char> pixels = CaptureScreenRect(x, y, w, h);

        std::string outText = ocr::get().GetImageText(pixels.data(), w, h);
        outText = trim(outText);

        resetWindow();

        setClipboard(outText);
    }

    if (isSelecting)
    {
        dragEnd = ImVec2((float)pt.x, (float)pt.y);
    }
}

void ui::resetWindow()
{
    ::ShowWindow(hWnd, SW_HIDE);
    dragStart = { -1.f, -1.f };
    dragEnd = { -1.f, -1.f };
    if (isSelecting)
        isSelecting = false;
}

void ui::drawDottedRect(ImVec2 topLeft, ImVec2 bottomRight)
{
    ImU32 light = IM_COL32(255, 255, 255, 200);
    ImU32 dark = IM_COL32(0, 0, 0, 200);
    float spacing = 4.0f;

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    RECT rect;
    GetWindowRect(hWnd, &rect);
    int windowX = rect.left;
    int windowY = rect.top;

    // Top edge
    for (float x = topLeft.x; x < bottomRight.x; x += spacing * 2)
    {
        int lum = pixelSampler->GetLuminanceWindowRelative((int)x, (int)topLeft.y + 1, windowX, windowY);
        ImU32 color = (lum >= 0 && lum < 128) ? light : dark;
        drawList->AddLine(ImVec2(x, topLeft.y), ImVec2(min(bottomRight.x, x + spacing), topLeft.y), color, 2.f);

    }

    // Bottom edge
    for (float x = topLeft.x; x < bottomRight.x; x += spacing * 2)
    {
        int lum = pixelSampler->GetLuminanceWindowRelative((int)x, (int)bottomRight.y - 1, windowX, windowY);
        ImU32 color = (lum >= 0 && lum < 128) ? light : dark;
        drawList->AddLine(ImVec2(x, bottomRight.y), ImVec2(min(bottomRight.x, x + spacing), bottomRight.y), color, 2.f);

    }

    // Left edge
    for (float y = topLeft.y; y < bottomRight.y; y += spacing * 2)
    {
        int lum = pixelSampler->GetLuminanceWindowRelative((int)topLeft.x + 1, (int)y, windowX, windowY);
        ImU32 color = (lum >= 0 && lum < 128) ? light : dark;
        drawList->AddLine(ImVec2(topLeft.x, y), ImVec2(topLeft.x, min(bottomRight.y, y + spacing)), color, 2.f);

    }

    // Right edge
    for (float y = topLeft.y; y < bottomRight.y; y += spacing * 2)
    {
        int lum = pixelSampler->GetLuminanceWindowRelative((int)bottomRight.x - 1, (int)y, windowX, windowY);
        ImU32 color = (lum >= 0 && lum < 128) ? light : dark;
        drawList->AddLine(ImVec2(bottomRight.x, y), ImVec2(bottomRight.x, min(bottomRight.y, y + spacing)), color, 2.f);

    }
}

void ui::drawSelection()
{
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
            else if (msg.message == WM_HOTKEY)
            {
                if (msg.wParam == 1)
                    ::ShowWindow(hWnd, SW_SHOWNOACTIVATE);
                else if (msg.wParam == 2)
                    resetWindow();
            }
        }
        if (done)
            break;

        handleMouse();
        

        if ((SwapChainOccluded && pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED))
        {
            ::Sleep(10);
            continue;
        }
        SwapChainOccluded = false;

        const float clear_color_with_alpha[4] = { 0.05f, 0.05f, 0.05f, 0.1f }; // fully transparent
        pd3dDeviceContext->OMSetRenderTargets(1, &mainRenderTargetView, nullptr);
        pd3dDeviceContext->ClearRenderTargetView(mainRenderTargetView, clear_color_with_alpha);            

        if (ResizeWidth != 0 && ResizeHeight != 0)
        {
            CleanupRenderTarget();
            pSwapChain->ResizeBuffers(0, ResizeWidth, ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            ResizeWidth = ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        {
            if (!(dragStart.x == -1) && !(dragStart.y == -1) && !(dragEnd.x == -1) && !(dragEnd.y == -1))
            {
                RECT rect;
                GetWindowRect(hWnd, &rect);
                int windowX = rect.left;
                int windowY = rect.top;

                ImVec2 topLeft = ImVec2(min(dragStart.x, dragEnd.x), min(dragStart.y, dragEnd.y));
                ImVec2 bottomRight = ImVec2(max(dragStart.x, dragEnd.x), max(dragStart.y, dragEnd.y));

                if (pixelSampler)
                    pixelSampler->Update(topLeft, bottomRight);

                topLeft.x -= windowX;
                topLeft.y -= windowY;
                bottomRight.x -= windowX;
                bottomRight.y -= windowY;

                drawDottedRect(topLeft, bottomRight);
            }
        }

        ImGui::Render();
        //const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = pSwapChain->Present(0, 0); // Present without vsync
        SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }
}

ui::~ui()
{
    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hWnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    UnregisterHotKey(hWnd, 1);
    UnregisterHotKey(hWnd, 2);

	if (pixelSampler)
        delete pixelSampler;
}

bool ui::CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &pSwapChain, &pd3dDevice, &featureLevel, &pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &pSwapChain, &pd3dDevice, &featureLevel, &pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void ui::CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (pSwapChain) { pSwapChain->Release(); pSwapChain = nullptr; }
    if (pd3dDeviceContext) { pd3dDeviceContext->Release(); pd3dDeviceContext = nullptr; }
    if (pd3dDevice) { pd3dDevice->Release(); pd3dDevice = nullptr; }
}

void ui::CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &mainRenderTargetView);
    pBackBuffer->Release();
}

void ui::CleanupRenderTarget()
{
    if (mainRenderTargetView) { mainRenderTargetView->Release(); mainRenderTargetView = nullptr; }
}


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

std::unique_ptr<ui> UI = std::make_unique<ui>();