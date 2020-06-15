// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

//#include <windows.h>
//#include <shlwapi.h>
//#include <strsafe.h>
//#include <new>
//#include <shlobj.h>
//#include "resource.h"

// Setup common controls v6 the easy way
//#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// window message to inform main window to close a file
//#define WM_FILEINUSE_CLOSEFILE (WM_USER + 1)

// this class implements the interface necessary to negotiate with the explorer
// when it hits sharing violations due to the file being open

class CFileIsInUseImpl : public IFileIsInUse
{
public:
	// IUnknown
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
	IFACEMETHODIMP_(ULONG) AddRef();
	IFACEMETHODIMP_(ULONG) Release();

	// IFileIsInUse
	IFACEMETHODIMP GetAppName(PWSTR *ppszName);
	IFACEMETHODIMP GetUsage(FILE_USAGE_TYPE *pfut);
	IFACEMETHODIMP GetCapabilities(DWORD *pdwCapabilitiesFlags);
	IFACEMETHODIMP GetSwitchToHWND(HWND *phwnd);
	IFACEMETHODIMP CloseFile();

	static HRESULT s_CreateInstance(HWND hwnd, LPCSTR pszFilePath, FILE_USAGE_TYPE fut, DWORD dwCapabilities, REFIID riid, void **ppv, DWORD *pdwCookie);

private:
	CFileIsInUseImpl();
	~CFileIsInUseImpl();

	HRESULT _Initialize(HWND hwnd, LPCSTR pszFilePath, FILE_USAGE_TYPE fut, DWORD dwCapabilities);
	HRESULT _AddFileToROT();
	HRESULT _RemoveFileFromROT();

	long _cRef;
	WCHAR _szFilePath[MAX_PATH];
	HWND _hwnd;
	DWORD _dwCapabilities;
	DWORD _dwCookie;
	FILE_USAGE_TYPE _fut;
};

