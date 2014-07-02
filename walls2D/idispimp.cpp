/*
 * idispimp.CPP
 * IDispatch for Extending Dynamic HTML Object Model
 *
 * Copyright (c)1995-2000 Microsoft Corporation, All Rights Reserved
 */

#include "stdafx.h"
#include <math.h>
#include "idispimp.h"
#include "walls2D.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// Hardcoded information for extending the Object Model 
// Typically this would be supplied through a TypeInfo
// In this case the name "xxyyzz" maps to DISPID_Extend

#define  NUM_FCNS 3

#define DISPID_Coords	54321	
#define DISPID_Title	54322		//keys
#define DISPID_Zoomed	54323		//keys

static WCHAR fcn_name[NUM_FCNS][10]={L"ccoooo",L"ttiitt",L"zzoooo"};


static UINT fcn_id[NUM_FCNS] = {DISPID_Coords,DISPID_Title,DISPID_Zoomed};

#define PI 3.141592653589793
#define U_DEGREES (PI/180.0)

/*
 * CImpIDispatch::CImpIDispatch
 * CImpIDispatch::~CImpIDispatch
 *
 * Parameters (Constructor):
 *  pSite           PCSite of the site we're in.
 *  pUnkOuter       LPUNKNOWN to which we delegate.
 */

CImpIDispatch::CImpIDispatch( void )
{
    m_cRef = 0;
}

CImpIDispatch::~CImpIDispatch( void )
{
	ASSERT( m_cRef == 0 );
}


/*
 * CImpIDispatch::QueryInterface
 * CImpIDispatch::AddRef
 * CImpIDispatch::Release
 *
 * Purpose:
 *  IUnknown members for CImpIDispatch object.
 */

STDMETHODIMP CImpIDispatch::QueryInterface( REFIID riid, void **ppv )
{
    *ppv = NULL;


    if ( IID_IDispatch == riid )
	{
        *ppv = this;
	}
	
	if ( NULL != *ppv )
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
	UINT	i,j;

	// Assume some degree of success
	hr = NOERROR;

	// Hardcoded mapping for this sample
	// A more usual procedure would be to use a TypeInfo
	for ( i=0; i < cNames; i++)
	{
		for(j=0;j<NUM_FCNS;j++) {
		  if(!memcmp((char*)fcn_name[j],(char*)rgszNames[i],12)) break;
		  //The following tests don't work for whatever reason!!!
		  //if(!lstrcmp((char*)fcn_name[j],(char*)rgszNames[i])) break;
		  //if(CSTR_EQUAL==CompareString(lcid,NORM_IGNOREWIDTH,(char*)fcn_name[j],-1,(char*)rgszNames[i],-1)) break;
		}

		if(j<NUM_FCNS) rgDispId[i]=fcn_id[j];
		else
		{
			// One or more are unknown so set the return code accordingly
			hr = ResultFromScode(DISP_E_UNKNOWNNAME);
			rgDispId[i] = DISPID_UNKNOWN;
		}
	}
	return hr;
}

#if 0
static void SetResult(DISPID dispIdMember,VARIANT* pVarResult)
{
	if (!pVarResult) return;
	VariantInit(pVarResult);

	V_VT(pVarResult)=VT_BOOL;
	V_UINT(pVarResult)=1;
}
#endif

static BOOL parse_georef(LPCSTR s,double *fView,double *xOff,double *yOff,double *fScale,double *eastOff,double *northOff)
{
	LPCSTR p;
	char cUnits;
	//Tamapatz Project  Plan: 90  Scale: 1:75  Frame: 17.15x13.33m  Center: 489311.3 2393778.12
	//theApp.m_csTitle
	if(p=strstr(s," Plan: ")) {
		ASSERT(theApp.m_pDocument!=NULL);
		theApp.m_pDocument->SetTitle(theApp.m_csTitle=CString(s,p-s));
		double view,scale,width,height,centerEast,centerNorth;
		int numval=sscanf(p+7,"%lf Scale: 1:%lf Frame: %lfx%lf%c Center: %lf %lf",
			&view,&scale,&width,&height,&cUnits,&centerEast,&centerNorth);
		if(numval==7) {
			//assign values --
			scale=((cUnits=='f')?(72*12.0):(72*12.0/0.3048))/scale;
			*fView=view*U_DEGREES;
			*eastOff=centerEast; *northOff=centerNorth;
			*xOff=width*0.5*scale; *yOff=height*0.5*scale;
			*fScale=1/scale;
			return TRUE;
		}
	}
	return FALSE;
}

STDMETHODIMP CImpIDispatch::Invoke(
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS* pDispParams,
            /* [out] */ VARIANT* pVarResult,
            /* [out] */ EXCEPINFO* /*pExcepInfo*/,
            /* [out] */ UINT* puArgErr)
{
	static double fsinA=0,fcosA=0;
	static double xOff=0,yOff=0;
	static double scale=0,eastOff=0,northOff=0;
	double t;

	// For this sample we only support a Property Get on DISPID_Extend
	// returning a BSTR with "Wibble" as the value

#if 0
	if (wFlags & DISPATCH_PROPERTYGET) SetResult(dispIdMember,pVarResult);
	else
#endif
	ASSERT(wFlags==DISPATCH_METHOD);
	VARIANTARG *pva=pDispParams->rgvarg;

	if(dispIdMember==DISPID_Coords) {
		ASSERT(pDispParams->cArgs==2);
		ASSERT(pva[0].vt==VT_R8 || pva[0].vt==VT_I4);
		ASSERT(pva[1].vt==VT_R8 || pva[1].vt==VT_I4);

		double x=(pva[1].vt==VT_R8)?pva[1].dblVal:(double)pva[1].intVal;
		double y=(pva[0].vt==VT_R8)?pva[0].dblVal:(double)pva[0].intVal;
		//(x,y) are the pointer's page coordinates.

		if(theApp.m_bCoordPage) {
			theApp.m_coord.x=x;
			theApp.m_coord.y=y;
			return S_OK;
		}
		//(xOff,yOff) are the (x,y) page coordinates of the center point.
		x-=xOff;
		y-=yOff;
		//(x,y) are now the pointer's page coordinates wrt center pt.
		t=y;
		y=-x*fsinA-y*fcosA;
		x=x*fcosA-t*fsinA;
		//Now (x,y)==(east,north) pointer coordinates in page units wrt to center pt,
		//which is at (eastOff,NorthOff) in final map units (feet or meters):
		theApp.m_coord.x=x*scale+eastOff;
		theApp.m_coord.y=y*scale+northOff;
	}
	else {
		if(dispIdMember==DISPID_Zoomed) {
			ASSERT(pDispParams->cArgs==1);
			ASSERT(pva[0].vt==VT_I4);
			int i=pva[0].intVal;
			theApp.m_bNotHomed=(i&1)!=0;
			theApp.m_bPanning=(i&2)!=0;
		}
		else if(dispIdMember==DISPID_Title) {
			ASSERT(pDispParams->cArgs==1);
			if(pva[0].vt==VT_BSTR) {
				CString s(pva[0].bstrVal);
				if(!parse_georef(s,&t,&xOff,&yOff,&scale,&eastOff,&northOff)) {
					AfxMessageBox(IDS_ERR_BADGEOREF);
					t=xOff=yOff=scale=eastOff=northOff=0.0;
				}
				fsinA=sin(t); fcosA=cos(t);
			}
		}
		else {
			ASSERT(FALSE);
		}
	}

	return S_OK;
}
