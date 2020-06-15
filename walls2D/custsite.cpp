
//=--------------------------------------------------------------------------=
//  (C) Copyright 1996-2000 Microsoft Corporation. All Rights Reserved.
//=--------------------------------------------------------------------------=


// 
// NOTE: 
// Some of the code in this file is MFC implementation specific.
// Changes in future versions of MFC implementation may require
// the code to be changed. Please check the readme of this
// sample for more information 
// 

#include "stdafx.h"
#undef AFX_DATA
#define AFX_DATA AFX_DATA_IMPORT


// NOTE: THis line is a hardcoded reference to an MFC header file
//  this path may need to be changed to refer to the location of VC5 install
//  for successful compilation.

//#include <ocdb.h>
//#include <occimpl.h>
#include <afxocc.h>

#undef AFX_DATA
#define AFX_DATA AFX_DATA_EXPORT
#include "custsite.h"
#include "walls2D.h"


BEGIN_INTERFACE_MAP(CCustomControlSite, COleControlSite)
	INTERFACE_PART(CCustomControlSite, IID_IDocHostUIHandler, DocHostUIHandler)
END_INTERFACE_MAP()



ULONG FAR EXPORT  CCustomControlSite::XDocHostUIHandler::AddRef()
{
	METHOD_PROLOGUE(CCustomControlSite, DocHostUIHandler)
		return pThis->ExternalAddRef();
}


ULONG FAR EXPORT  CCustomControlSite::XDocHostUIHandler::Release()
{
	METHOD_PROLOGUE(CCustomControlSite, DocHostUIHandler)
		return pThis->ExternalRelease();
}

HRESULT FAR EXPORT  CCustomControlSite::XDocHostUIHandler::QueryInterface(REFIID riid, void **ppvObj)
{
	METHOD_PROLOGUE(CCustomControlSite, DocHostUIHandler)
		HRESULT hr = (HRESULT)pThis->ExternalQueryInterface(&riid, ppvObj);
	return hr;
}

// * CImpIDocHostUIHandler::GetHostInfo
// *
// * Purpose: Called at initialization
// *
HRESULT FAR EXPORT  CCustomControlSite::XDocHostUIHandler::GetHostInfo(DOCHOSTUIINFO* pInfo)
{

	METHOD_PROLOGUE(CCustomControlSite, DocHostUIHandler)
		pInfo->dwFlags = DOCHOSTUIFLAG_NO3DBORDER;
	pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;

	return S_OK;
}

// * CImpIDocHostUIHandler::ShowUI
// *
// * Purpose: Called when MSHTML.DLL shows its UI
// *
HRESULT FAR EXPORT  CCustomControlSite::XDocHostUIHandler::ShowUI(
	DWORD dwID,
	IOleInPlaceActiveObject * /*pActiveObject*/,
	IOleCommandTarget * pCommandTarget,
	IOleInPlaceFrame * /*pFrame*/,
	IOleInPlaceUIWindow * /*pDoc*/)
{

	METHOD_PROLOGUE(CCustomControlSite, DocHostUIHandler)
		// We've already got our own UI in place so just return S_OK
		return S_OK;
}

// * CImpIDocHostUIHandler::HideUI
// *
// * Purpose: Called when MSHTML.DLL hides its UI
// *
HRESULT FAR EXPORT  CCustomControlSite::XDocHostUIHandler::HideUI(void)
{
	METHOD_PROLOGUE(CCustomControlSite, DocHostUIHandler)
		return S_OK;
}

// * CImpIDocHostUIHandler::UpdateUI
// *
// * Purpose: Called when MSHTML.DLL updates its UI
// *
HRESULT FAR EXPORT  CCustomControlSite::XDocHostUIHandler::UpdateUI(void)
{
	METHOD_PROLOGUE(CCustomControlSite, DocHostUIHandler)
		// MFC is pretty good about updating it's UI in it's Idle loop so I don't do anything here
		return S_OK;
}

// * CImpIDocHostUIHandler::EnableModeless
// *
// * Purpose: Called from MSHTML.DLL's IOleInPlaceActiveObject::EnableModeless
// *
HRESULT FAR EXPORT  CCustomControlSite::XDocHostUIHandler::EnableModeless(BOOL /*fEnable*/)
{
	METHOD_PROLOGUE(CCustomControlSite, DocHostUIHandler)
		return E_NOTIMPL;
}

// * CImpIDocHostUIHandler::OnDocWindowActivate
// *
// * Purpose: Called from MSHTML.DLL's IOleInPlaceActiveObject::OnDocWindowActivate
// *
HRESULT FAR EXPORT  CCustomControlSite::XDocHostUIHandler::OnDocWindowActivate(BOOL /*fActivate*/)
{
	METHOD_PROLOGUE(CCustomControlSite, DocHostUIHandler)
		return E_NOTIMPL;
}

// * CImpIDocHostUIHandler::OnFrameWindowActivate
// *
// * Purpose: Called from MSHTML.DLL's IOleInPlaceActiveObject::OnFrameWindowActivate
// *
HRESULT FAR EXPORT  CCustomControlSite::XDocHostUIHandler::OnFrameWindowActivate(BOOL /*fActivate*/)
{
	METHOD_PROLOGUE(CCustomControlSite, DocHostUIHandler)
		return E_NOTIMPL;
}

// * CImpIDocHostUIHandler::ResizeBorder
// *
// * Purpose: Called from MSHTML.DLL's IOleInPlaceActiveObject::ResizeBorder
// *
HRESULT FAR EXPORT  CCustomControlSite::XDocHostUIHandler::ResizeBorder(
	LPCRECT /*prcBorder*/,
	IOleInPlaceUIWindow* /*pUIWindow*/,
	BOOL /*fRameWindow*/)
{
	METHOD_PROLOGUE(CCustomControlSite, DocHostUIHandler)
		return E_NOTIMPL;
}

// * CImpIDocHostUIHandler::ShowContextMenu
// *
// * Purpose: Called when MSHTML.DLL would normally display its context menu
// *
HRESULT FAR EXPORT  CCustomControlSite::XDocHostUIHandler::ShowContextMenu(
	DWORD id/*dwID*/,
	POINT* /*pptPosition*/,
	IUnknown* /*pCommandTarget*/,
	IDispatch* /*pDispatchObjectHit*/)
{
	METHOD_PROLOGUE(CCustomControlSite, DocHostUIHandler)
		//return S_OK; // We've shown our own context menu. MSHTML.DLL will no longer try to show its own.
		return (id == 0) ? S_OK : S_FALSE;
}

// * CImpIDocHostUIHandler::TranslateAccelerator
// *
// * Purpose: Called from MSHTML.DLL's TranslateAccelerator routines
// *
HRESULT FAR EXPORT  CCustomControlSite::XDocHostUIHandler::TranslateAccelerator(LPMSG lpMsg,
	/* [in] */ const GUID __RPC_FAR *pguidCmdGroup,
	/* [in] */ DWORD nCmdID)
{
	METHOD_PROLOGUE(CCustomControlSite, DocHostUIHandler)
		return S_FALSE;
}

// * CImpIDocHostUIHandler::GetOptionKeyPath
// *
// * Purpose: Called by MSHTML.DLL to find where the host wishes to store 
// *	its options in the registry
// *
HRESULT FAR EXPORT  CCustomControlSite::XDocHostUIHandler::GetOptionKeyPath(BSTR* pbstrKey, DWORD)
{

	METHOD_PROLOGUE(CCustomControlSite, DocHostUIHandler)
		return E_NOTIMPL;
}

STDMETHODIMP CCustomControlSite::XDocHostUIHandler::GetDropTarget(
	/* [in] */ IDropTarget __RPC_FAR *pDropTarget,
	/* [out] */ IDropTarget __RPC_FAR *__RPC_FAR *ppDropTarget)
{
	METHOD_PROLOGUE(CCustomControlSite, DocHostUIHandler)
		return E_NOTIMPL;
}

STDMETHODIMP CCustomControlSite::XDocHostUIHandler::GetExternal(
	/* [out] */ IDispatch __RPC_FAR *__RPC_FAR *ppDispatch)
{
	// return the IDispatch we have for extending the object Model
	IDispatch* pDisp = (IDispatch*)theApp.m_pDispOM;
	pDisp->AddRef();
	*ppDispatch = pDisp;
	return S_OK;
}

STDMETHODIMP CCustomControlSite::XDocHostUIHandler::TranslateUrl(
	/* [in] */ DWORD dwTranslate,
	/* [in] */ OLECHAR __RPC_FAR *pchURLIn,
	/* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppchURLOut)
{
	METHOD_PROLOGUE(CCustomControlSite, DocHostUIHandler)
		return E_NOTIMPL;
}

STDMETHODIMP CCustomControlSite::XDocHostUIHandler::FilterDataObject(
	/* [in] */ IDataObject __RPC_FAR *pDO,
	/* [out] */ IDataObject __RPC_FAR *__RPC_FAR *ppDORet)
{
	METHOD_PROLOGUE(CCustomControlSite, DocHostUIHandler)
		return E_NOTIMPL;
}

