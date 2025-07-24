#pragma once

#include <tesseract/baseapi.h>
#include <memory>
#include <string>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class ocr
{
public:
	static ocr& get()
	{
		static ocr instance;
		return instance;
	}

	std::string GetImageText(unsigned char* pixelData, int width, int heigth);

private:
	ocr() = default;

	// Optional: prevent copy/move
	ocr(const ocr&) = delete;
	ocr& operator=(const ocr&) = delete;
	ocr(ocr&&) = delete;
	ocr& operator=(ocr&&) = delete;
};

