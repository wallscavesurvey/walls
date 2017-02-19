// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "stdafx.h"
#include <shlwapi.h>
#include <strsafe.h>
#include <new>
#include <shlobj.h>
#include "resource.h"
#include "FileIsInUse.h"

// Setup common controls v6 the easy way
//#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// window message to inform main window to close a file
//#define WM_FILEINUSE_CLOSEFILE (WM_USER + 1)

// this class implements the interface necessary to negotiate with the explorer
// when it hits sharing violations due to the file being open

//*** class CFileIsInUseImpl : public IFileIsInUse

CFileIsInUseImpl::CFileIsInUseImpl(): _cRef(1), _hwnd(NULL), _fut(FUT_GENERIC), _dwCapabilities(0), _dwCookie(0)
{
    _szFilePath[0]  = '\0';
}

CFileIsInUseImpl::~CFileIsInUseImpl()
{
    _RemoveFileFromROT();
}

HRESULT CFileIsInUseImpl::_Initialize(HWND hwnd, LPCSTR pszFilePath, FILE_USAGE_TYPE fut, DWORD dwCapabilities)
{
	USES_CONVERSION; //required for T2COLE macro

    _hwnd  = hwnd;
    _fut   = fut;
    _dwCapabilities = dwCapabilities;
    HRESULT hr = StringCchCopyW(_szFilePath, ARRAYSIZE(_szFilePath),A2W(pszFilePath));
    if (SUCCEEDED(hr))
    {
        hr = _AddFileToROT();
    }
    return hr;
}

HRESULT CFileIsInUseImpl::s_CreateInstance(HWND hwnd, LPCSTR pszFilePath, FILE_USAGE_TYPE fut, DWORD dwCapabilities, REFIID riid, void **ppv, DWORD *pdwCookie)
{
    CFileIsInUseImpl *pfiu = new (std::nothrow) CFileIsInUseImpl();
    HRESULT hr = (pfiu) ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        hr = pfiu->_Initialize(hwnd, pszFilePath, fut, dwCapabilities);
        if (SUCCEEDED(hr))
        {
            hr = pfiu->QueryInterface(riid, ppv);
			if(SUCCEEDED(hr))
				*pdwCookie=pfiu->_dwCookie;
        }
        pfiu->Release();
    }
    return hr;
}

HRESULT CFileIsInUseImpl::QueryInterface(REFIID riid, void **ppv)
{
	//avoid warning C4838 -- converting OFFSETOFCLASS() to int --
    #define QITABENTMULTI_XX(Cthis, Ifoo, Iimpl) { &__uuidof(Ifoo), (int)OFFSETOFCLASS(Iimpl, Cthis) }
	#define QITABENT_XX(Cthis, Ifoo) QITABENTMULTI_XX(Cthis, Ifoo, Ifoo)

    static const QITAB qit[] = {QITABENT_XX(CFileIsInUseImpl, IFileIsInUse), { 0, 0 } };
    return QISearch(this, qit, riid, ppv);
}

ULONG CFileIsInUseImpl::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CFileIsInUseImpl::Release()
{
    ULONG cRef = InterlockedDecrement(&_cRef);
    if (!cRef)
    {
        delete this;
    }
    return cRef;
}

// IFileIsInUse

HRESULT CFileIsInUseImpl::CloseFile()
{
    // Notify main application window that we need to close
    // the file handle associated with this entry.  We do
    // not pass anything with this message since this sample
    // will only have a single file open at a time.

    //SendMessage(_hwnd, WM_FILEINUSE_CLOSEFILE, (WPARAM)NULL, (LPARAM)NULL);
	ASSERT(0); //won't use this capability
    //_RemoveFileFromROT();
    return S_OK;
}

// IFileIsInUse

HRESULT CFileIsInUseImpl::GetAppName(PWSTR *ppszName)
{
    HRESULT hr = E_FAIL;
    WCHAR szModule[MAX_PATH];
    UINT cch = GetModuleFileNameW(NULL, szModule, ARRAYSIZE(szModule));
    if (cch != 0)
    {
        hr = SHStrDupW(PathFindFileNameW(szModule), ppszName);
    }
    return hr;
}

// IFileIsInUse

HRESULT CFileIsInUseImpl::GetUsage(FILE_USAGE_TYPE *pfut)
{
    *pfut = _fut;
    return S_OK;
}

// IFileIsInUse

HRESULT CFileIsInUseImpl::GetCapabilities(DWORD *pdwCapabilitiesFlags)
{
    *pdwCapabilitiesFlags = _dwCapabilities;
    return S_OK;
}

// IFileIsInUse

HRESULT CFileIsInUseImpl::GetSwitchToHWND(HWND *phwnd)
{
    *phwnd = _hwnd;
    return S_OK;
}

HRESULT CFileIsInUseImpl::_AddFileToROT()
{
    IRunningObjectTable *prot;
    HRESULT hr = GetRunningObjectTable(NULL, &prot);
    if (SUCCEEDED(hr))
    {
        IMoniker *pmk;
        hr = CreateFileMoniker(_szFilePath, &pmk);
        if (SUCCEEDED(hr))
        {
            // Add ROTFLAGS_ALLOWANYCLIENT to make this work accross security boundaries
            // hr = prot->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE | ROTFLAGS_ALLOWANYCLIENT,static_cast<IFileIsInUse *>(this), pmk, &_dwCookie);
            // if (hr == CO_E_WRONG_SERVER_IDENTITY)
            //{
                // this failure is due to ROTFLAGS_ALLOWANYCLIENT and the fact that we don't
                // have the AppID registered for our CLSID. Try again without ROTFLAGS_ALLOWANYCLIENT
                // knowing that this means this can only work in the scope of apps running with the
                // same MIC level.
            hr = prot->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE,static_cast<IFileIsInUse *>(this), pmk, &_dwCookie);
            //}
            pmk->Release();
        }
        prot->Release();
    }
    return hr;
}

HRESULT CFileIsInUseImpl::_RemoveFileFromROT()
{
    IRunningObjectTable *prot;
    HRESULT hr = GetRunningObjectTable(NULL, &prot);
    if (SUCCEEDED(hr))
    {
        if (_dwCookie)
        {
            hr = prot->Revoke(_dwCookie);
            _dwCookie = 0;
        }

        prot->Release();
    }
    return hr;
}
