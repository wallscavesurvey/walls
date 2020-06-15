#include <windows.h>

int APIENTRY WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	HMODULE hModule = GetModuleHandleW(NULL);

	TCHAR path[MAX_PATH];
	GetModuleFileName(hModule, path, MAX_PATH);
	LPTSTR p = path + strlen(path);
	while (p > path && p[-1] != '\\') p--;
	if (!strcpy_s(p, MAX_PATH - (p - path), "Walls32.exe")) {
		if ((int)ShellExecute(NULL, "open", path, lpCmdLine, NULL, SW_NORMAL) > 32)
			return 0;
	}
	return -1;
}
