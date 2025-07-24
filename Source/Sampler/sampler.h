#pragma once

#include <d3d11.h>
#include <vector>
#include <stdint.h>
#include <dxgi.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "../../ImGui/imgui.h"

class PixelSampler {
public:
    PixelSampler() {
        hScreenDC = GetDC(nullptr); // desktop device context
        hMemDC = CreateCompatibleDC(hScreenDC);
        width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

        hBitmap = nullptr;
    }

    ~PixelSampler() {
        if (hBitmap) DeleteObject(hBitmap);
        if (hMemDC) DeleteDC(hMemDC);
        if (hScreenDC) ReleaseDC(nullptr, hScreenDC);
    }

    void Update(const ImVec2& topLeft, const ImVec2& bottomRight)
    {
        // Compute screen-space coordinates
        int x1 = static_cast<int>(topLeft.x);
        int y1 = static_cast<int>(topLeft.y);
        int x2 = static_cast<int>(bottomRight.x);
        int y2 = static_cast<int>(bottomRight.y);

        int captureWidth = x2 - x1;
        int captureHeight = y2 - y1;

        if (captureWidth <= 0 || captureHeight <= 0)
            return;

        // Clean up old bitmap
        if (hBitmap) {
            DeleteObject(hBitmap);
            hBitmap = nullptr;
        }

        hBitmap = CreateCompatibleBitmap(hScreenDC, captureWidth, captureHeight);
        SelectObject(hMemDC, hBitmap);

        // Capture from absolute screen-space
        BitBlt(hMemDC, 0, 0, captureWidth, captureHeight, hScreenDC, x1, y1, SRCCOPY);

        // Store capture area
        captureOffsetX = x1;
        captureOffsetY = y1;
        captureWidth_ = captureWidth;
        captureHeight_ = captureHeight;
    }

    // --- Screen-space input ---
    int GetLuminanceAt(int screenX, int screenY) const {
        int localX = screenX - captureOffsetX;
        int localY = screenY - captureOffsetY;

        if (localX < 0 || localY < 0 || localX >= captureWidth_ || localY >= captureHeight_)
            return -1;

        COLORREF color = GetPixel(hMemDC, localX, localY);
        if (color == CLR_INVALID)
            return -1;

        BYTE r = GetRValue(color);
        BYTE g = GetGValue(color);
        BYTE b = GetBValue(color);

        return static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);
    }

    // --- ImGui-relative input ---
    int GetLuminanceWindowRelative(int windowRelativeX, int windowRelativeY, int windowX, int windowY) const {
        return GetLuminanceAt(windowRelativeX + windowX, windowRelativeY + windowY);
    }

private:
    HDC hScreenDC = nullptr;
    HDC hMemDC = nullptr;
    HBITMAP hBitmap = nullptr;
    int width = 0, height = 0;
    int captureOffsetX = 0, captureOffsetY = 0;
    int captureWidth_ = 0, captureHeight_ = 0;
};