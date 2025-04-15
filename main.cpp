#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <string>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

HWND overlayWindow = nullptr;
HWND contrastWindow = nullptr;
HWND sliderWindow = nullptr;
HWND slotButtons[3] = { nullptr };
HWND resetButton = nullptr;

int alphaValue = 100;
int gammaLevel = 200;
int contrastAlpha = 40;
int colorTempLevel = 128;
int currentSlot = 1;

HFONT hSmallFont;

COLORREF GetColorFromTemperature(int value) {
    int r = (255 * value + 200 * (255 - value)) / 255;
    int g = (240 * value + 220 * (255 - value)) / 255;
    int b = (200 * value + 255 * (255 - value)) / 255;
    return RGB(r, g, b);
}

void UpdateOverlayAlpha() {
    SetLayeredWindowAttributes(overlayWindow, 0, (BYTE)alphaValue, LWA_ALPHA);
}

void UpdateOverlayColor() {
    COLORREF col = GetColorFromTemperature(colorTempLevel);
    int gray = gammaLevel;
    int r = min(255, GetRValue(col) * gray / 255);
    int g = min(255, GetGValue(col) * gray / 255);
    int b = min(255, GetBValue(col) * gray / 255);

    HBRUSH newBrush = CreateSolidBrush(RGB(r, g, b));
    SetClassLongPtr(overlayWindow, GCLP_HBRBACKGROUND, (LONG_PTR)newBrush);
    InvalidateRect(overlayWindow, NULL, TRUE);
}

void UpdateContrastAlpha() {
    SetLayeredWindowAttributes(contrastWindow, 0, (BYTE)contrastAlpha, LWA_ALPHA);
}

void SaveCurrentSettings() {
    wchar_t path[MAX_PATH];
    GetModuleFileName(nullptr, path, MAX_PATH);
    PathRemoveFileSpec(path);
    wcscat_s(path, L"\\config.ini");

    wchar_t section[16];
    swprintf_s(section, L"Slot%d", currentSlot);

    wchar_t value[16];
    swprintf_s(value, L"%d", alphaValue);
    WritePrivateProfileString(section, L"Brightness", value, path);
    swprintf_s(value, L"%d", gammaLevel);
    WritePrivateProfileString(section, L"Gamma", value, path);
    swprintf_s(value, L"%d", contrastAlpha);
    WritePrivateProfileString(section, L"Contrast", value, path);
    swprintf_s(value, L"%d", colorTempLevel);
    WritePrivateProfileString(section, L"ColorTemp", value, path);
}

void LoadSettings(int slot) {
    currentSlot = slot;

    wchar_t path[MAX_PATH];
    GetModuleFileName(nullptr, path, MAX_PATH);
    PathRemoveFileSpec(path);
    wcscat_s(path, L"\\config.ini");

    wchar_t section[16];
    swprintf_s(section, L"Slot%d", slot);

    alphaValue = GetPrivateProfileInt(section, L"Brightness", 100, path);
    gammaLevel = GetPrivateProfileInt(section, L"Gamma", 200, path);
    contrastAlpha = GetPrivateProfileInt(section, L"Contrast", 40, path);
    colorTempLevel = GetPrivateProfileInt(section, L"ColorTemp", 128, path);
}

void UpdateUIControls() {
    SendMessage(GetDlgItem(sliderWindow, 1), TBM_SETPOS, TRUE, alphaValue);
    SendMessage(GetDlgItem(sliderWindow, 2), TBM_SETPOS, TRUE, gammaLevel);
    SendMessage(GetDlgItem(sliderWindow, 3), TBM_SETPOS, TRUE, contrastAlpha);
    SendMessage(GetDlgItem(sliderWindow, 4), TBM_SETPOS, TRUE, colorTempLevel);
    UpdateOverlayAlpha();
    UpdateOverlayColor();
    UpdateContrastAlpha();
}

void ResetSliders() {
    alphaValue = 0;
    gammaLevel = 120;
    contrastAlpha = 0;
    colorTempLevel = 0;
    UpdateUIControls();
    SaveCurrentSettings();
}

LRESULT CALLBACK SliderWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND:
        if (wParam >= 1001 && wParam <= 1003) {
            LoadSettings((int)wParam - 1000);
            UpdateUIControls();
        }
        else if (wParam == 2000) {
            ResetSliders();
        }
        break;
    case WM_HOTKEY:
        switch (wParam) {
        case 1: LoadSettings(1); UpdateUIControls(); break; // Alt + 1
        case 2: LoadSettings(2); UpdateUIControls(); break; // Alt + 2
        case 3: LoadSettings(3); UpdateUIControls(); break; // Alt + 3
        case 4: LoadSettings(1); UpdateUIControls(); break; // F1
        case 5: LoadSettings(2); UpdateUIControls(); break; // F2
        case 6: LoadSettings(3); UpdateUIControls(); break; // F3
        case 7: PostQuitMessage(0); break; // END key
        }
        break;
    case WM_HSCROLL:
        if ((HWND)lParam == GetDlgItem(hwnd, 1)) {
            alphaValue = SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
            UpdateOverlayAlpha();
        }
        else if ((HWND)lParam == GetDlgItem(hwnd, 2)) {
            gammaLevel = SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
            UpdateOverlayColor();
        }
        else if ((HWND)lParam == GetDlgItem(hwnd, 3)) {
            contrastAlpha = SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
            UpdateContrastAlpha();
        }
        else if ((HWND)lParam == GetDlgItem(hwnd, 4)) {
            colorTempLevel = SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
            UpdateOverlayColor();
        }
        SaveCurrentSettings();
        break;
    case WM_DESTROY:
        for (int i = 1; i <= 7; i++) {
            UnregisterHotKey(nullptr, i);
        }
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    InitCommonControls();

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    hSmallFont = CreateFont(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    const wchar_t* contrastClass = L"ContrastOverlayWindow";
    WNDCLASS wcContrast = {};
    wcContrast.lpfnWndProc = DefWindowProc;
    wcContrast.hInstance = hInstance;
    wcContrast.lpszClassName = contrastClass;
    wcContrast.hbrBackground = CreateSolidBrush(RGB(30, 30, 30));
    RegisterClass(&wcContrast);

    contrastWindow = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        contrastClass, L"", WS_POPUP, 0, 0, screenWidth, screenHeight,
        nullptr, nullptr, hInstance, nullptr);

    const wchar_t* overlayClass = L"OverlayWindow";
    WNDCLASS wcOverlay = {};
    wcOverlay.lpfnWndProc = DefWindowProc;
    wcOverlay.hInstance = hInstance;
    wcOverlay.lpszClassName = overlayClass;
    wcOverlay.hbrBackground = CreateSolidBrush(RGB(200, 200, 200));
    RegisterClass(&wcOverlay);

    overlayWindow = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        overlayClass, L"", WS_POPUP, 0, 0, screenWidth, screenHeight,
        nullptr, nullptr, hInstance, nullptr);

    const wchar_t* sliderClass = L"SliderWindow";
    WNDCLASS wcSlider = {};
    wcSlider.lpfnWndProc = SliderWndProc;
    wcSlider.hInstance = hInstance;
    wcSlider.lpszClassName = sliderClass;
    wcSlider.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcSlider.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    RegisterClass(&wcSlider);

    sliderWindow = CreateWindowEx(0, sliderClass, L"ButachanFilter v1.1",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        100, 100, 320, 300, nullptr, nullptr, hInstance, nullptr);

    for (int i = 0; i < 3; i++) {
        slotButtons[i] = CreateWindow(L"BUTTON", std::to_wstring(i + 1).c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10 + i * 50, 10, 40, 25,
            sliderWindow, (HMENU)(1001 + i), hInstance, nullptr);
        SendMessage(slotButtons[i], WM_SETFONT, (WPARAM)hSmallFont, TRUE);
    }

    resetButton = CreateWindow(L"BUTTON", L"リセット",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        170, 10, 60, 25,
        sliderWindow, (HMENU)2000, hInstance, nullptr);
    SendMessage(resetButton, WM_SETFONT, (WPARAM)hSmallFont, TRUE);

    RegisterHotKey(sliderWindow, 1, MOD_ALT, 0x31); // Alt + 1
    RegisterHotKey(sliderWindow, 2, MOD_ALT, 0x32); // Alt + 2
    RegisterHotKey(sliderWindow, 3, MOD_ALT, 0x33); // Alt + 3
    RegisterHotKey(sliderWindow, 4, 0, VK_F1);
    RegisterHotKey(sliderWindow, 5, 0, VK_F2);
    RegisterHotKey(sliderWindow, 6, 0, VK_F3);
    RegisterHotKey(sliderWindow, 7, 0, VK_END); // END

    LoadSettings(1);
    UpdateOverlayAlpha();
    UpdateOverlayColor();
    UpdateContrastAlpha();
    ShowWindow(contrastWindow, SW_SHOW);
    ShowWindow(overlayWindow, SW_SHOW);
    ShowWindow(sliderWindow, SW_SHOW);

    int y = 40;
    auto createLabel = [&](LPCWSTR text, int y) {
        HWND h = CreateWindowEx(WS_EX_TRANSPARENT, L"STATIC", text,
            WS_CHILD | WS_VISIBLE | SS_LEFT, 10, y, 280, 20,
            sliderWindow, nullptr, hInstance, nullptr);
        SendMessage(h, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
        };

    auto createSlider = [&](int id, int y, int min, int max, int value) {
        HWND h = CreateWindowEx(0, TRACKBAR_CLASS, L"",
            WS_CHILD | WS_VISIBLE,
            10, y, 280, 30,
            sliderWindow, (HMENU)id, hInstance, nullptr);
        SendMessage(h, TBM_SETRANGE, TRUE, MAKELPARAM(min, max));
        SendMessage(h, TBM_SETPOS, TRUE, value);
        };

    createLabel(L"明るさ", y); createSlider(1, y + 20, 0, 255, alphaValue); y += 50;
    createLabel(L"ガンマ", y); createSlider(2, y + 20, 120, 255, gammaLevel); y += 50;
    createLabel(L"コントラスト", y); createSlider(3, y + 20, 0, 255, contrastAlpha); y += 50;
    createLabel(L"色温度", y); createSlider(4, y + 20, 0, 255, colorTempLevel);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
