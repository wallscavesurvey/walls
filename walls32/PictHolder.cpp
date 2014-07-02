// PictureHolderEx.cpp: implementation of
// the CPictureHolderEx class.
//
///////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "PictHolder.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// copied from MFC source
#ifndef RELEASE
#define RELEASE(lpUnk) \
    do { if ((lpUnk) != NULL) { (lpUnk)->Release(); \
(lpUnk) = NULL; } } while (0)
#endif

///////////////////////////////////////////////////////////////
// Construction/Destruction
///////////////////////////////////////////////////////////////

CPictureHolderEx::CPictureHolderEx()
: CPictureHolder()
{
    // nothing to do
}

/*
CPictureHolderEx::CPictureHolderEx(const CPictureHolderEx& holder)
: CPictureHolder() // there is no copy c'tor for CPictureHolder
{
    // copy and bump up the reference count
    // so the IPicture knows it has an extra pointer
    m_pPict = holder.m_pPict;
    if (m_pPict) m_pPict->AddRef();
}

CPictureHolderEx&
CPictureHolderEx::operator=(const CPictureHolderEx& from)
{
    if (this != &from) {
        // copy and bump up the reference count
        // so the IPicture knows it has an extra pointer
        // make sure we release the old picture too
        IPicture* pPict = m_pPict;
        m_pPict = from.m_pPict;
        if (m_pPict) m_pPict->AddRef();
        RELEASE(pPict);
    }

    return *this;
}
*/
///////////////////////////////////////////////////////////////
// Handles
///////////////////////////////////////////////////////////////

/*
OLE_HANDLE CPictureHolderEx::GetHandle() {
    OLE_HANDLE handle = NULL;
    if (m_pPict != NULL) {
        m_pPict->get_Handle(&handle);
    }
    return handle;
}
*/

///////////////////////////////////////////////////////////////
// Load Picture
///////////////////////////////////////////////////////////////

//JDS{}{}{} the maximum length parameter is new.  Note the default parameter
//bool OleLoadPictureFromGlobal(HGLOBAL hGlobal, int maxLen = 0);

bool CPictureHolderEx::OleLoadPictureFromGlobal(HGLOBAL hGlobal, int maxLen) {
 if (! hGlobal) return false;

 // create IStream* from global memory
 // (the TRUE means deleted on release)
 LPSTREAM lpStream = NULL;
 try {
     HRESULT hr = ::CreateStreamOnHGlobal(hGlobal, FALSE, &lpStream);
     ASSERT(SUCCEEDED(hr) && lpStream);
     if (! SUCCEEDED(hr) || ! lpStream) return false;
 } catch (...) {
     return false;
 }

 // load the picture from the stream
 bool bLoaded;
 try {
    bLoaded = OleLoadPictureFromStream(lpStream,maxLen);
 } catch (...) {
    bLoaded = false;
 }

 try {
     RELEASE(lpStream);
     return bLoaded;
 } catch (...) {
     return false;
 }
}

//JDS{}{}{} the maximum length parameter is new.  Note the default parameter
//bool OleLoadPictureFromStream(LPSTREAM lpStream, int maxLen = 0);

bool CPictureHolderEx::OleLoadPictureFromStream(LPSTREAM lpStream, int maxLen) {
 // simply call ::OleLoadPicture to read it in
 //JDS{}{}{} the try/catch block is new.  We get access violations on
 //invalid files sometimes.  Other times it displays as much as it can,
 //and on the very same files!
 try {
     if (! lpStream) return false;

     HRESULT hr;
     hr = ::OleLoadPicture(lpStream, maxLen, FALSE,IID_IPicture,(LPVOID*)&m_pPict);

     return SUCCEEDED(hr);

 } catch (...) {
     return false;
 }
}

//m_pictureHolder.OleLoadPictureFromResource(IDR_JPGS1, _T("JPGs"));

bool CPictureHolderEx::OleLoadPictureFromResource(UINT nID, LPCTSTR lpszType, HINSTANCE hInstResource/*=NULL*/)
{
	HMODULE hMod = (HMODULE) hInstResource;
	if ( hInstResource == NULL )
		hMod = (HMODULE) ::AfxGetResourceHandle();
	HRSRC hResInfo = ::FindResource(hMod, MAKEINTRESOURCE(nID), lpszType);
	ASSERT(hResInfo!=NULL);

	HGLOBAL hRes      = ::LoadResource(hMod, hResInfo);
	DWORD   dwSize    = ::SizeofResource(hMod, hResInfo);
	LPVOID  pvDataRes = ::LockResource(hRes);

	HGLOBAL hGlobal   = ::GlobalAlloc(GMEM_MOVEABLE, dwSize);
	ASSERT(hGlobal!=NULL);

	if (! hGlobal) return false;

	LPVOID  pvData = ::GlobalLock(hGlobal);
	::memcpy(pvData,pvDataRes,dwSize);
	::GlobalUnlock(hGlobal);

	bool bLoaded = OleLoadPictureFromGlobal(hGlobal, dwSize);

	::GlobalFree(hGlobal);
	::FreeResource(hRes);

	return bLoaded;
}
/*
bool CPictureHolderEx::GetDimensions(CSize& size)
{
	if (!HasPicture() || !IsBitmap())
		return false;
	HBITMAP handle = (HBITMAP)*this;
	DIBSECTION dibsection;
	if (0==GetObject(handle,sizeof(DIBSECTION),&dibsection))
		return false;
	size.cx = dibsection.dsBmih.biWidth;
	size.cy = dibsection.dsBmih.biHeight;
	return true;
}
*/