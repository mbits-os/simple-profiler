#include <Windows.h>


extern "C" void win32_setIcon(HWND hwnd)
{
	HINSTANCE inst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
	HICON icon = LoadIcon(inst, MAKEINTRESOURCE(101));
	SetClassLongPtr(hwnd, GCLP_HICON, (LONG_PTR)icon);

	icon = (HICON)LoadImage(inst, MAKEINTRESOURCE(101), IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
		LR_DEFAULTCOLOR);
	SetClassLongPtr(hwnd, GCLP_HICONSM, (LONG_PTR)icon);
}
