#ifndef _USERWINDOW
#define _USERWINDOW

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windef.h>

class UserWindow{
public:
	HWND Window;
	WNDCLASS WinClass;
    const wchar_t* ClassName;
    LRESULT (*WindowProc)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    UserWindow(const wchar_t* class_name) : Window(NULL), ClassName(class_name) { WinClass = { 0 }; }

	bool init (
        const wchar_t* WinName, 
        HWND ParentWindow,
        DWORD WinStyle, 
        int x = CW_USEDEFAULT,
        int y = CW_USEDEFAULT,
        int Width = CW_USEDEFAULT,
        int Height = CW_USEDEFAULT,
        DWORD ExWinStyle = 0, 
        HMENU Menu = 0,  
        LPVOID AppData = 0);

};
#endif