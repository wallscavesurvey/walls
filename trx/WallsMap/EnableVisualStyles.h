
#ifndef _ENABLEVISUALSTYLES_H
#define _ENABLEVISUALSTYLES_H

#ifndef _INC_SHLWAPI
#include <shlwapi.h>	// IsThemeEnabled
#endif

bool IsCommonControlsEnabled();
bool IsThemeEnabled();
LRESULT EnableWindowTheme(HWND hwnd, LPCWSTR classList, LPCWSTR subApp, LPCWSTR idlist);
bool EnableVisualStyles(HWND hwnd,bool bValue); //true if successful
#endif
