#include <Windows.h>


extern "C" void win32_setIcon(HWND hwnd)
{
    HINSTANCE inst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
    HICON icon = LoadIcon(inst, MAKEINTRESOURCE(101));
    SetClassLongPtr(hwnd, GCLP_HICON, (LONG_PTR)icon);
}
