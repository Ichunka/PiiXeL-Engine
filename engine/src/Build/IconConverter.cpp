#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <string>
#include <vector>
#include <windows.h>

namespace PiiXeL {

struct IconPixelData {
    std::vector<unsigned char> pixels;
    int width;
    int height;
};

bool ConvertIcoToRawPixels(const std::string& icoPath, IconPixelData& outData) {
    HICON hIcon =
        static_cast<HICON>(::LoadImageA(nullptr, icoPath.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE));

    if (!hIcon)
    { return false; }

    ICONINFO iconInfo;
    if (!::GetIconInfo(hIcon, &iconInfo))
    {
        ::DestroyIcon(hIcon);
        return false;
    }

    BITMAP bmp;
    if (!::GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp))
    {
        ::DeleteObject(iconInfo.hbmColor);
        ::DeleteObject(iconInfo.hbmMask);
        ::DestroyIcon(hIcon);
        return false;
    }

    int width = bmp.bmWidth;
    int height = bmp.bmHeight;

    HDC hdc = ::GetDC(nullptr);
    HDC memDC = ::CreateCompatibleDC(hdc);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    outData.pixels.resize(width * height * 4);
    outData.width = width;
    outData.height = height;

    bool success = false;
    if (::GetDIBits(memDC, iconInfo.hbmColor, 0, height, outData.pixels.data(), &bmi, DIB_RGB_COLORS))
    {
        for (int i = 0; i < width * height; i++)
        {
            unsigned char b = outData.pixels[i * 4 + 0];
            unsigned char g = outData.pixels[i * 4 + 1];
            unsigned char r = outData.pixels[i * 4 + 2];
            unsigned char a = outData.pixels[i * 4 + 3];

            outData.pixels[i * 4 + 0] = r;
            outData.pixels[i * 4 + 1] = g;
            outData.pixels[i * 4 + 2] = b;
            outData.pixels[i * 4 + 3] = a;
        }
        success = true;
    }

    ::DeleteDC(memDC);
    ::ReleaseDC(nullptr, hdc);
    ::DeleteObject(iconInfo.hbmColor);
    ::DeleteObject(iconInfo.hbmMask);
    ::DestroyIcon(hIcon);

    return success;
}

} // namespace PiiXeL

#endif
