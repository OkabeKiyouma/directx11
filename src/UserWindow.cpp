#include "UserWindow.h"

bool UserWindow::init(
    const wchar_t* WinName,
    HWND ParentWindow,
    DWORD WinStyle,
    int x,
    int y,
    int Width,
    int Height,
    DWORD ExWinStyle,
    HMENU Menu,
    LPVOID AppData) {
    WinClass.lpfnWndProc = WindowProc;
    WinClass.hInstance = GetModuleHandle(NULL);
    WinClass.lpszClassName = ClassName;

    RegisterClass(&WinClass);

    Window = CreateWindowEx(
        ExWinStyle, ClassName, WinName, WinStyle, x, y,
        Width, Height, ParentWindow, Menu, GetModuleHandle(NULL), AppData
    );

    return (Window ? TRUE : FALSE);
}