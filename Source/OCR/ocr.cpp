#include "ocr.h"

std::string ocr::GetImageText(unsigned char* pixelData, int width, int heigth)
{
    tesseract::TessBaseAPI tess;

    if (tess.Init(NULL, "deu+eng", tesseract::OEM_TESSERACT_LSTM_COMBINED) != 0)
        MessageBoxA(NULL, "Error initializing tesseract", "", MB_OK);

    tess.SetImage(pixelData, width, heigth, 4, width * 4);

    if (tess.MeanTextConf() == 0)
        MessageBoxA(NULL, "OCR confidence is 0 – likely invalid image", "", MB_OK);

    char* t = tess.GetUTF8Text();

    std::wstring wOutText;
    int len = MultiByteToWideChar(CP_UTF8, 0, t, -1, nullptr, 0);
    if (len > 0)
    {
        wOutText.resize(len - 1);
        MultiByteToWideChar(CP_UTF8, 0, t, -1, &wOutText[0], len);
    }
    delete t;

    std::string outText(wOutText.begin(), wOutText.end());
    return outText;
}
