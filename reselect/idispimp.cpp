/*
 * idispimp.CPP
 * IDispatch for Extending Dynamic HTML Object Model
 *
 * Copyright (c)1995-2000 Microsoft Corporation, All Rights Reserved
 */

#include "stdafx.h"
#include "idispimp.h"
#include "reselect.h"
#include "resellib.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// Hardcoded information for extending the Object Model 
// Typically this would be supplied through a TypeInfo
// In this case the name "xxyyzz" maps to DISPID_Extend

#define  NUM_FCNS 12

#define DISPID_Texbib	54321		//keys
#define DISPID_Texbibc	54322		//showkey radio
#define DISPID_Texbibcc 54323		//showno check
#define DISPID_Texbiboo 54324		//and/or
#define DISPID_Texbiba	54325		//names
#define DISPID_Texbibdd 54326

#define DISPID_Nwbb		64321
#define DISPID_Nwbbc	64322
#define DISPID_Nwbbcc	64323
#define DISPID_Nwbboo	64324
#define DISPID_Nwbba	64325
#define DISPID_Nwbbdd	64326

static WCHAR fcn_name[NUM_FCNS][12] = {
	L"tteexx",L"ccttee",L"tteecc",L"oottee",L"aattee",L"tteedd",
	L"nnwwbb",L"ccnnww",L"nnwwcc",L"oonnww",L"aannww",L"nnwwdd" };

static UINT fcn_id[NUM_FCNS] = {
	DISPID_Texbib,DISPID_Texbibc,DISPID_Texbibcc,DISPID_Texbiboo,DISPID_Texbiba,DISPID_Texbibdd,
	DISPID_Nwbb,DISPID_Nwbbc,DISPID_Nwbbcc,DISPID_Nwbboo,DISPID_Nwbba,DISPID_Nwbbdd };

/*
 * CImpIDispatch::CImpIDispatch
 * CImpIDispatch::~CImpIDispatch
 *
 * Parameters (Constructor):
 *  pSite           PCSite of the site we're in.
 *  pUnkOuter       LPUNKNOWN to which we delegate.
 */

CImpIDispatch::CImpIDispatch(void)
{
	m_cRef = 0;
}

CImpIDispatch::~CImpIDispatch(void)
{
	ASSERT(m_cRef == 0);
}


/*
 * CImpIDispatch::QueryInterface
 * CImpIDispatch::AddRef
 * CImpIDispatch::Release
 *
 * Purpose:
 *  IUnknown members for CImpIDispatch object.
 */

STDMETHODIMP CImpIDispatch::QueryInterface(REFIID riid, void **ppv)
{
	*ppv = NULL;


	if (IID_IDispatch == riid)
	{
		*ppv = this;
	}

	if (NULL != *ppv)
	{
		((LPUNKNOWN)*ppv)->AddRef();
		return NOERROR;
	}

	return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CImpIDispatch::AddRef(void)
{
	return ++m_cRef;
}

STDMETHODIMP_(ULONG) CImpIDispatch::Release(void)
{
	return --m_cRef;
}


//IDispatch
STDMETHODIMP CImpIDispatch::GetTypeInfoCount(UINT* /*pctinfo*/)
{
	return E_NOTIMPL;
}

STDMETHODIMP CImpIDispatch::GetTypeInfo(/* [in] */ UINT /*iTInfo*/,
	/* [in] */ LCID /*lcid*/,
	/* [out] */ ITypeInfo** /*ppTInfo*/)
{
	return E_NOTIMPL;
}

STDMETHODIMP CImpIDispatch::GetIDsOfNames(
	/* [in] */ REFIID riid,
	/* [size_is][in] */ OLECHAR** rgszNames,
	/* [in] */ UINT cNames,
	/* [in] */ LCID lcid,
	/* [size_is][out] */ DISPID* rgDispId)
{
	HRESULT hr;
	UINT	i, j;

	// Assume some degree of success
	hr = NOERROR;

	// Hardcoded mapping for this sample
	// A more usual procedure would be to use a TypeInfo
	for (i = 0; i < cNames; i++)
	{
		for (j = 0; j < NUM_FCNS; j++) {
			if (!memcmp((char*)fcn_name[j], (char*)rgszNames[i], 12)) break;
			//The following tests don't work for whatever reason!!!
			//if(!lstrcmp((char*)fcn_name[j],(char*)rgszNames[i])) break;
			//if(CSTR_EQUAL==CompareString(lcid,NORM_IGNOREWIDTH,(char*)fcn_name[j],-1,(char*)rgszNames[i],-1)) break;
		}

		if (j < NUM_FCNS) rgDispId[i] = fcn_id[j];
		else
		{
			// One or more are unknown so set the return code accordingly
			hr = ResultFromScode(DISP_E_UNKNOWNNAME);
			rgDispId[i] = DISPID_UNKNOWN;
		}
	}
	return hr;
}

static void SetResult(DISPID dispIdMember, VARIANT* pVarResult)
{
	if (!pVarResult) return;

	VariantInit(pVarResult);

	switch (dispIdMember) {
	case DISPID_Texbib:
	case DISPID_Nwbb:
	{
		CString buffer =
			(dispIdMember == DISPID_Texbib) ? (char *)theApp.m_texbib_keybuf : (char *)theApp.m_nwbb_keybuf;
		BSTR buffer_ = buffer.AllocSysString();
		V_VT(pVarResult) = VT_BSTR;
		V_BSTR(pVarResult) = buffer_;
	}
	break;

	case DISPID_Texbiba:
	case DISPID_Nwbba:
	{
		CString buffer =
			(dispIdMember == DISPID_Texbiba) ? (char *)theApp.m_texbib_nambuf : (char *)theApp.m_nwbb_nambuf;
		BSTR buffer_ = buffer.AllocSysString();
		V_VT(pVarResult) = VT_BSTR;
		V_BSTR(pVarResult) = buffer_;
	}
	break;

	case DISPID_Texbiboo:
	case DISPID_Nwbboo:
		V_VT(pVarResult) = VT_UINT;
		V_UINT(pVarResult) = (0 != (RESEL_OR_AUTHOR_MASK &
			((dispIdMember == DISPID_Texbiboo) ? (theApp.m_texbib_bFlags) : (theApp.m_nwbb_bFlags))));
		break;
		break;

	case DISPID_Texbibc:
	case DISPID_Nwbbc:
		V_VT(pVarResult) = VT_UINT;
		V_UINT(pVarResult) = (RESEL_SHOWKEY_MASK &
			((dispIdMember == DISPID_Texbibc) ? (theApp.m_texbib_bFlags) : (theApp.m_nwbb_bFlags)));
		break;

	case DISPID_Texbibcc:
	case DISPID_Nwbbcc:
		V_VT(pVarResult) = VT_BOOL;
		V_UINT(pVarResult) = (0 != (RESEL_SHOWNO_MASK &
			((dispIdMember == DISPID_Texbibcc) ? (theApp.m_texbib_bFlags) : (theApp.m_nwbb_bFlags))));
		break;

	case DISPID_Texbibdd:
	case DISPID_Nwbbdd:
		V_VT(pVarResult) = VT_BOOL;
		V_UINT(pVarResult) = (0 != (RESEL_SORT_MASK &
			((dispIdMember == DISPID_Texbibdd) ? (theApp.m_texbib_bFlags) : (theApp.m_nwbb_bFlags))));
		break;
	}
}

STDMETHODIMP CImpIDispatch::Invoke(
	/* [in] */ DISPID dispIdMember,
	/* [in] */ REFIID /*riid*/,
	/* [in] */ LCID /*lcid*/,
	/* [in] */ WORD wFlags,
	/* [out][in] */ DISPPARAMS* pDispParams,
	/* [out] */ VARIANT* pVarResult,
	/* [out] */ EXCEPINFO* /*pExcepInfo*/,
	/* [out] */ UINT* puArgErr)
{

	// For this sample we only support a Property Get on DISPID_Extend
	// returning a BSTR with "Wibble" as the value

	if (wFlags & DISPATCH_PROPERTYGET) SetResult(dispIdMember, pVarResult);

	return S_OK;
}
