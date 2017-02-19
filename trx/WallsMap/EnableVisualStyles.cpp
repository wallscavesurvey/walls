#include "stdafx.h"
#ifdef _USE_THEMES
#include "EnableVisualStyles.h"

bool IsCommonControlsEnabled()
{
	static DWORD dllVersion=0; 

	// Test if application has access to common controls
	if(dllVersion) return dllVersion>=6;

	dllVersion=1;

	HMODULE hinstDll = ::LoadLibrary(_T("comctl32.dll"));
	if (hinstDll)
	{
		DLLGETVERSIONPROC pDllGetVersion = (DLLGETVERSIONPROC)::GetProcAddress(hinstDll, "DllGetVersion");
		::FreeLibrary(hinstDll);
		if (pDllGetVersion != NULL)
		{
			DLLVERSIONINFO dvi = {0};
			dvi.cbSize = sizeof(dvi);
			HRESULT hRes = pDllGetVersion ((DLLVERSIONINFO *) &dvi);
			if (SUCCEEDED(hRes)) {
				return (dllVersion=dvi.dwMajorVersion)>=6;
			}
		}
	}
	return false;
}

bool IsThemeEnabled()
{
	if(!IsCommonControlsEnabled()) return false;

	HMODULE hinstDll;

	// Test if operating system has themes enabled
	hinstDll = ::LoadLibrary(_T("UxTheme.dll"));
	if (hinstDll)
	{
		bool (__stdcall *pIsAppThemed)();
		bool (__stdcall *pIsThemeActive)();
		(FARPROC&)pIsAppThemed = ::GetProcAddress(hinstDll, "IsAppThemed");
		(FARPROC&)pIsThemeActive = ::GetProcAddress(hinstDll,"IsThemeActive");
		::FreeLibrary(hinstDll);
		if (pIsAppThemed != NULL && pIsThemeActive != NULL)
		{
			return pIsAppThemed() && pIsThemeActive();
		}
	}
	return false;
}

LRESULT EnableWindowTheme(HWND hwnd, LPCWSTR classList, LPCWSTR subApp, LPCWSTR idlist)
{
	HMODULE hinstDll;
	HRESULT (__stdcall *pSetWindowTheme)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);
	HANDLE (__stdcall *pOpenThemeData)(HWND hwnd, LPCWSTR pszClassList);
	HRESULT (__stdcall *pCloseThemeData)(HANDLE hTheme);

	hinstDll = ::LoadLibrary(_T("UxTheme.dll"));
	if (hinstDll)
	{
		(FARPROC&)pOpenThemeData = ::GetProcAddress(hinstDll, "OpenThemeData");
		(FARPROC&)pCloseThemeData = ::GetProcAddress(hinstDll, "CloseThemeData");
		(FARPROC&)pSetWindowTheme = ::GetProcAddress(hinstDll, "SetWindowTheme");
		::FreeLibrary(hinstDll);
		if (pSetWindowTheme && pOpenThemeData && pCloseThemeData)
		{
			HANDLE theme = pOpenThemeData(hwnd,classList);
			if (theme!=NULL)
			{
				VERIFY(pCloseThemeData(theme)==S_OK);
				return pSetWindowTheme(hwnd, subApp, idlist);
			}
			//return pSetWindowTheme(hwnd, L" ", L" "); returns 0 !? 
		}
	}
	return S_FALSE;
}

bool EnableVisualStyles(HWND hwnd,bool bValue)
{
	if (wOSVersion < 0x0500 || !IsThemeEnabled())
	{
		return !bValue;
	}

	LRESULT rc;
	if (bValue)
		rc = EnableWindowTheme(hwnd, L"ListView", L"Explorer", NULL);
	else
		rc = EnableWindowTheme(hwnd, L"", L"", NULL);

	return rc==S_OK;
}
#endif