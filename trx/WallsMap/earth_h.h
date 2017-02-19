

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0366 */
/* at Wed Dec 26 13:11:03 2007
 */
/* Compiler settings for .\earth.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __earth_h_h__
#define __earth_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ITimeGE_FWD_DEFINED__
#define __ITimeGE_FWD_DEFINED__
typedef interface ITimeGE ITimeGE;
#endif 	/* __ITimeGE_FWD_DEFINED__ */


#ifndef __ITimeIntervalGE_FWD_DEFINED__
#define __ITimeIntervalGE_FWD_DEFINED__
typedef interface ITimeIntervalGE ITimeIntervalGE;
#endif 	/* __ITimeIntervalGE_FWD_DEFINED__ */


#ifndef __ICameraInfoGE_FWD_DEFINED__
#define __ICameraInfoGE_FWD_DEFINED__
typedef interface ICameraInfoGE ICameraInfoGE;
#endif 	/* __ICameraInfoGE_FWD_DEFINED__ */


#ifndef __ITourControllerGE_FWD_DEFINED__
#define __ITourControllerGE_FWD_DEFINED__
typedef interface ITourControllerGE ITourControllerGE;
#endif 	/* __ITourControllerGE_FWD_DEFINED__ */


#ifndef __ISearchControllerGE_FWD_DEFINED__
#define __ISearchControllerGE_FWD_DEFINED__
typedef interface ISearchControllerGE ISearchControllerGE;
#endif 	/* __ISearchControllerGE_FWD_DEFINED__ */


#ifndef __IAnimationControllerGE_FWD_DEFINED__
#define __IAnimationControllerGE_FWD_DEFINED__
typedef interface IAnimationControllerGE IAnimationControllerGE;
#endif 	/* __IAnimationControllerGE_FWD_DEFINED__ */


#ifndef __IViewExtentsGE_FWD_DEFINED__
#define __IViewExtentsGE_FWD_DEFINED__
typedef interface IViewExtentsGE IViewExtentsGE;
#endif 	/* __IViewExtentsGE_FWD_DEFINED__ */


#ifndef __IFeatureGE_FWD_DEFINED__
#define __IFeatureGE_FWD_DEFINED__
typedef interface IFeatureGE IFeatureGE;
#endif 	/* __IFeatureGE_FWD_DEFINED__ */


#ifndef __IFeatureCollectionGE_FWD_DEFINED__
#define __IFeatureCollectionGE_FWD_DEFINED__
typedef interface IFeatureCollectionGE IFeatureCollectionGE;
#endif 	/* __IFeatureCollectionGE_FWD_DEFINED__ */


#ifndef __IPointOnTerrainGE_FWD_DEFINED__
#define __IPointOnTerrainGE_FWD_DEFINED__
typedef interface IPointOnTerrainGE IPointOnTerrainGE;
#endif 	/* __IPointOnTerrainGE_FWD_DEFINED__ */


#ifndef __IApplicationGE_FWD_DEFINED__
#define __IApplicationGE_FWD_DEFINED__
typedef interface IApplicationGE IApplicationGE;
#endif 	/* __IApplicationGE_FWD_DEFINED__ */


#ifndef __IKHViewInfo_FWD_DEFINED__
#define __IKHViewInfo_FWD_DEFINED__
typedef interface IKHViewInfo IKHViewInfo;
#endif 	/* __IKHViewInfo_FWD_DEFINED__ */


#ifndef __IKHViewExtents_FWD_DEFINED__
#define __IKHViewExtents_FWD_DEFINED__
typedef interface IKHViewExtents IKHViewExtents;
#endif 	/* __IKHViewExtents_FWD_DEFINED__ */


#ifndef __IKHFeature_FWD_DEFINED__
#define __IKHFeature_FWD_DEFINED__
typedef interface IKHFeature IKHFeature;
#endif 	/* __IKHFeature_FWD_DEFINED__ */


#ifndef __IKHInterface_FWD_DEFINED__
#define __IKHInterface_FWD_DEFINED__
typedef interface IKHInterface IKHInterface;
#endif 	/* __IKHInterface_FWD_DEFINED__ */


#ifndef __ApplicationGE_FWD_DEFINED__
#define __ApplicationGE_FWD_DEFINED__

#ifdef __cplusplus
typedef class ApplicationGE ApplicationGE;
#else
typedef struct ApplicationGE ApplicationGE;
#endif /* __cplusplus */

#endif 	/* __ApplicationGE_FWD_DEFINED__ */


#ifndef __TimeGE_FWD_DEFINED__
#define __TimeGE_FWD_DEFINED__

#ifdef __cplusplus
typedef class TimeGE TimeGE;
#else
typedef struct TimeGE TimeGE;
#endif /* __cplusplus */

#endif 	/* __TimeGE_FWD_DEFINED__ */


#ifndef __TimeIntervalGE_FWD_DEFINED__
#define __TimeIntervalGE_FWD_DEFINED__

#ifdef __cplusplus
typedef class TimeIntervalGE TimeIntervalGE;
#else
typedef struct TimeIntervalGE TimeIntervalGE;
#endif /* __cplusplus */

#endif 	/* __TimeIntervalGE_FWD_DEFINED__ */


#ifndef __CameraInfoGE_FWD_DEFINED__
#define __CameraInfoGE_FWD_DEFINED__

#ifdef __cplusplus
typedef class CameraInfoGE CameraInfoGE;
#else
typedef struct CameraInfoGE CameraInfoGE;
#endif /* __cplusplus */

#endif 	/* __CameraInfoGE_FWD_DEFINED__ */


#ifndef __ViewExtentsGE_FWD_DEFINED__
#define __ViewExtentsGE_FWD_DEFINED__

#ifdef __cplusplus
typedef class ViewExtentsGE ViewExtentsGE;
#else
typedef struct ViewExtentsGE ViewExtentsGE;
#endif /* __cplusplus */

#endif 	/* __ViewExtentsGE_FWD_DEFINED__ */


#ifndef __TourControllerGE_FWD_DEFINED__
#define __TourControllerGE_FWD_DEFINED__

#ifdef __cplusplus
typedef class TourControllerGE TourControllerGE;
#else
typedef struct TourControllerGE TourControllerGE;
#endif /* __cplusplus */

#endif 	/* __TourControllerGE_FWD_DEFINED__ */


#ifndef __SearchControllerGE_FWD_DEFINED__
#define __SearchControllerGE_FWD_DEFINED__

#ifdef __cplusplus
typedef class SearchControllerGE SearchControllerGE;
#else
typedef struct SearchControllerGE SearchControllerGE;
#endif /* __cplusplus */

#endif 	/* __SearchControllerGE_FWD_DEFINED__ */


#ifndef __AnimationControllerGE_FWD_DEFINED__
#define __AnimationControllerGE_FWD_DEFINED__

#ifdef __cplusplus
typedef class AnimationControllerGE AnimationControllerGE;
#else
typedef struct AnimationControllerGE AnimationControllerGE;
#endif /* __cplusplus */

#endif 	/* __AnimationControllerGE_FWD_DEFINED__ */


#ifndef __FeatureGE_FWD_DEFINED__
#define __FeatureGE_FWD_DEFINED__

#ifdef __cplusplus
typedef class FeatureGE FeatureGE;
#else
typedef struct FeatureGE FeatureGE;
#endif /* __cplusplus */

#endif 	/* __FeatureGE_FWD_DEFINED__ */


#ifndef __FeatureCollectionGE_FWD_DEFINED__
#define __FeatureCollectionGE_FWD_DEFINED__

#ifdef __cplusplus
typedef class FeatureCollectionGE FeatureCollectionGE;
#else
typedef struct FeatureCollectionGE FeatureCollectionGE;
#endif /* __cplusplus */

#endif 	/* __FeatureCollectionGE_FWD_DEFINED__ */


#ifndef __PointOnTerrainGE_FWD_DEFINED__
#define __PointOnTerrainGE_FWD_DEFINED__

#ifdef __cplusplus
typedef class PointOnTerrainGE PointOnTerrainGE;
#else
typedef struct PointOnTerrainGE PointOnTerrainGE;
#endif /* __cplusplus */

#endif 	/* __PointOnTerrainGE_FWD_DEFINED__ */


#ifndef __KHInterface_FWD_DEFINED__
#define __KHInterface_FWD_DEFINED__

#ifdef __cplusplus
typedef class KHInterface KHInterface;
#else
typedef struct KHInterface KHInterface;
#endif /* __cplusplus */

#endif 	/* __KHInterface_FWD_DEFINED__ */


#ifndef __KHViewInfo_FWD_DEFINED__
#define __KHViewInfo_FWD_DEFINED__

#ifdef __cplusplus
typedef class KHViewInfo KHViewInfo;
#else
typedef struct KHViewInfo KHViewInfo;
#endif /* __cplusplus */

#endif 	/* __KHViewInfo_FWD_DEFINED__ */


#ifndef __KHViewExtents_FWD_DEFINED__
#define __KHViewExtents_FWD_DEFINED__

#ifdef __cplusplus
typedef class KHViewExtents KHViewExtents;
#else
typedef struct KHViewExtents KHViewExtents;
#endif /* __cplusplus */

#endif 	/* __KHViewExtents_FWD_DEFINED__ */


#ifndef __KHFeature_FWD_DEFINED__
#define __KHFeature_FWD_DEFINED__

#ifdef __cplusplus
typedef class KHFeature KHFeature;
#else
typedef struct KHFeature KHFeature;
#endif /* __cplusplus */

#endif 	/* __KHFeature_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_earth_0000 */
/* [local] */ 

#include <winerror.h>
#define E_INVALID_OR_DELETED_FEATURE     MAKE_HRESULT(SEVERITY_ERROR,    FACILITY_ITF, 0x8001)
#define E_APPLICATION_UNINITIALIZED      MAKE_HRESULT(SEVERITY_ERROR,    FACILITY_ITF, 0x8002)
#define S_TELEPORTED                     MAKE_HRESULT(SEVERITY_SUCCESS,  FACILITY_ITF, 0x8003)
#define E_FEATURE_HAS_NO_VIEW            MAKE_HRESULT(SEVERITY_ERROR,    FACILITY_ITF, 0x8004)
#define E_USAGE_LIMIT_EXCEEDED           MAKE_HRESULT(SEVERITY_ERROR,    FACILITY_ITF, 0x8005)
#define E_INFINITE_NUMBER                MAKE_HRESULT(SEVERITY_ERROR,    FACILITY_ITF, 0x8006)











typedef /* [public][public][public][public][v1_enum][uuid] */  DECLSPEC_UUID("ECF071D1-75F4-4F61-A5D9-D96EEAF5EC1D") 
enum __MIDL___MIDL_itf_earth_0000_0001
    {	RelativeToGroundAltitudeGE	= 1,
	AbsoluteAltitudeGE	= 2
    } 	AltitudeModeGE;



extern RPC_IF_HANDLE __MIDL_itf_earth_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_earth_0000_v0_0_s_ifspec;

#ifndef __ITimeGE_INTERFACE_DEFINED__
#define __ITimeGE_INTERFACE_DEFINED__

/* interface ITimeGE */
/* [unique][helpstring][dual][uuid][object] */ 

typedef /* [public][public][public][v1_enum] */ 
enum __MIDL_ITimeGE_0002
    {	TimeNegativeInfinityGE	= -1,
	TimeFiniteGE	= 0,
	TimePositiveInfinityGE	= 1
    } 	TimeTypeGE;


EXTERN_C const IID IID_ITimeGE;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E39391AE-51C0-4FBD-9042-F9C5B6094445")
    ITimeGE : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ TimeTypeGE *pType) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Type( 
            /* [in] */ TimeTypeGE type) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Year( 
            /* [retval][out] */ int *pYear) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Year( 
            /* [in] */ int year) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Month( 
            /* [retval][out] */ int *pMonth) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Month( 
            /* [in] */ int month) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Day( 
            /* [retval][out] */ int *pDay) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Day( 
            /* [in] */ int day) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Hour( 
            /* [retval][out] */ int *pHour) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Hour( 
            /* [in] */ int hour) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Minute( 
            /* [retval][out] */ int *pMinute) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Minute( 
            /* [in] */ int minute) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Second( 
            /* [retval][out] */ int *pSecond) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Second( 
            /* [in] */ int second) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TimeZone( 
            /* [retval][out] */ double *pTimeZone) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_TimeZone( 
            /* [in] */ double timeZone) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ ITimeGE **pTime) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ConvertToZone( 
            /* [in] */ double timeZone,
            /* [retval][out] */ ITimeGE **pTime) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITimeGEVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITimeGE * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITimeGE * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITimeGE * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITimeGE * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITimeGE * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITimeGE * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITimeGE * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Type )( 
            ITimeGE * This,
            /* [retval][out] */ TimeTypeGE *pType);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Type )( 
            ITimeGE * This,
            /* [in] */ TimeTypeGE type);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Year )( 
            ITimeGE * This,
            /* [retval][out] */ int *pYear);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Year )( 
            ITimeGE * This,
            /* [in] */ int year);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Month )( 
            ITimeGE * This,
            /* [retval][out] */ int *pMonth);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Month )( 
            ITimeGE * This,
            /* [in] */ int month);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Day )( 
            ITimeGE * This,
            /* [retval][out] */ int *pDay);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Day )( 
            ITimeGE * This,
            /* [in] */ int day);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Hour )( 
            ITimeGE * This,
            /* [retval][out] */ int *pHour);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Hour )( 
            ITimeGE * This,
            /* [in] */ int hour);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Minute )( 
            ITimeGE * This,
            /* [retval][out] */ int *pMinute);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Minute )( 
            ITimeGE * This,
            /* [in] */ int minute);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Second )( 
            ITimeGE * This,
            /* [retval][out] */ int *pSecond);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Second )( 
            ITimeGE * This,
            /* [in] */ int second);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TimeZone )( 
            ITimeGE * This,
            /* [retval][out] */ double *pTimeZone);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TimeZone )( 
            ITimeGE * This,
            /* [in] */ double timeZone);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ITimeGE * This,
            /* [retval][out] */ ITimeGE **pTime);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ConvertToZone )( 
            ITimeGE * This,
            /* [in] */ double timeZone,
            /* [retval][out] */ ITimeGE **pTime);
        
        END_INTERFACE
    } ITimeGEVtbl;

    interface ITimeGE
    {
        CONST_VTBL struct ITimeGEVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITimeGE_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITimeGE_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITimeGE_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITimeGE_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITimeGE_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITimeGE_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITimeGE_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITimeGE_get_Type(This,pType)	\
    (This)->lpVtbl -> get_Type(This,pType)

#define ITimeGE_put_Type(This,type)	\
    (This)->lpVtbl -> put_Type(This,type)

#define ITimeGE_get_Year(This,pYear)	\
    (This)->lpVtbl -> get_Year(This,pYear)

#define ITimeGE_put_Year(This,year)	\
    (This)->lpVtbl -> put_Year(This,year)

#define ITimeGE_get_Month(This,pMonth)	\
    (This)->lpVtbl -> get_Month(This,pMonth)

#define ITimeGE_put_Month(This,month)	\
    (This)->lpVtbl -> put_Month(This,month)

#define ITimeGE_get_Day(This,pDay)	\
    (This)->lpVtbl -> get_Day(This,pDay)

#define ITimeGE_put_Day(This,day)	\
    (This)->lpVtbl -> put_Day(This,day)

#define ITimeGE_get_Hour(This,pHour)	\
    (This)->lpVtbl -> get_Hour(This,pHour)

#define ITimeGE_put_Hour(This,hour)	\
    (This)->lpVtbl -> put_Hour(This,hour)

#define ITimeGE_get_Minute(This,pMinute)	\
    (This)->lpVtbl -> get_Minute(This,pMinute)

#define ITimeGE_put_Minute(This,minute)	\
    (This)->lpVtbl -> put_Minute(This,minute)

#define ITimeGE_get_Second(This,pSecond)	\
    (This)->lpVtbl -> get_Second(This,pSecond)

#define ITimeGE_put_Second(This,second)	\
    (This)->lpVtbl -> put_Second(This,second)

#define ITimeGE_get_TimeZone(This,pTimeZone)	\
    (This)->lpVtbl -> get_TimeZone(This,pTimeZone)

#define ITimeGE_put_TimeZone(This,timeZone)	\
    (This)->lpVtbl -> put_TimeZone(This,timeZone)

#define ITimeGE_Clone(This,pTime)	\
    (This)->lpVtbl -> Clone(This,pTime)

#define ITimeGE_ConvertToZone(This,timeZone,pTime)	\
    (This)->lpVtbl -> ConvertToZone(This,timeZone,pTime)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITimeGE_get_Type_Proxy( 
    ITimeGE * This,
    /* [retval][out] */ TimeTypeGE *pType);


void __RPC_STUB ITimeGE_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITimeGE_put_Type_Proxy( 
    ITimeGE * This,
    /* [in] */ TimeTypeGE type);


void __RPC_STUB ITimeGE_put_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITimeGE_get_Year_Proxy( 
    ITimeGE * This,
    /* [retval][out] */ int *pYear);


void __RPC_STUB ITimeGE_get_Year_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITimeGE_put_Year_Proxy( 
    ITimeGE * This,
    /* [in] */ int year);


void __RPC_STUB ITimeGE_put_Year_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITimeGE_get_Month_Proxy( 
    ITimeGE * This,
    /* [retval][out] */ int *pMonth);


void __RPC_STUB ITimeGE_get_Month_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITimeGE_put_Month_Proxy( 
    ITimeGE * This,
    /* [in] */ int month);


void __RPC_STUB ITimeGE_put_Month_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITimeGE_get_Day_Proxy( 
    ITimeGE * This,
    /* [retval][out] */ int *pDay);


void __RPC_STUB ITimeGE_get_Day_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITimeGE_put_Day_Proxy( 
    ITimeGE * This,
    /* [in] */ int day);


void __RPC_STUB ITimeGE_put_Day_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITimeGE_get_Hour_Proxy( 
    ITimeGE * This,
    /* [retval][out] */ int *pHour);


void __RPC_STUB ITimeGE_get_Hour_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITimeGE_put_Hour_Proxy( 
    ITimeGE * This,
    /* [in] */ int hour);


void __RPC_STUB ITimeGE_put_Hour_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITimeGE_get_Minute_Proxy( 
    ITimeGE * This,
    /* [retval][out] */ int *pMinute);


void __RPC_STUB ITimeGE_get_Minute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITimeGE_put_Minute_Proxy( 
    ITimeGE * This,
    /* [in] */ int minute);


void __RPC_STUB ITimeGE_put_Minute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITimeGE_get_Second_Proxy( 
    ITimeGE * This,
    /* [retval][out] */ int *pSecond);


void __RPC_STUB ITimeGE_get_Second_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITimeGE_put_Second_Proxy( 
    ITimeGE * This,
    /* [in] */ int second);


void __RPC_STUB ITimeGE_put_Second_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITimeGE_get_TimeZone_Proxy( 
    ITimeGE * This,
    /* [retval][out] */ double *pTimeZone);


void __RPC_STUB ITimeGE_get_TimeZone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITimeGE_put_TimeZone_Proxy( 
    ITimeGE * This,
    /* [in] */ double timeZone);


void __RPC_STUB ITimeGE_put_TimeZone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITimeGE_Clone_Proxy( 
    ITimeGE * This,
    /* [retval][out] */ ITimeGE **pTime);


void __RPC_STUB ITimeGE_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITimeGE_ConvertToZone_Proxy( 
    ITimeGE * This,
    /* [in] */ double timeZone,
    /* [retval][out] */ ITimeGE **pTime);


void __RPC_STUB ITimeGE_ConvertToZone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITimeGE_INTERFACE_DEFINED__ */


#ifndef __ITimeIntervalGE_INTERFACE_DEFINED__
#define __ITimeIntervalGE_INTERFACE_DEFINED__

/* interface ITimeIntervalGE */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ITimeIntervalGE;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D794FE36-10B1-4E7E-959D-9638794D2A1B")
    ITimeIntervalGE : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_BeginTime( 
            /* [retval][out] */ ITimeGE **pTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EndTime( 
            /* [retval][out] */ ITimeGE **pTime) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITimeIntervalGEVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITimeIntervalGE * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITimeIntervalGE * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITimeIntervalGE * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITimeIntervalGE * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITimeIntervalGE * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITimeIntervalGE * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITimeIntervalGE * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BeginTime )( 
            ITimeIntervalGE * This,
            /* [retval][out] */ ITimeGE **pTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EndTime )( 
            ITimeIntervalGE * This,
            /* [retval][out] */ ITimeGE **pTime);
        
        END_INTERFACE
    } ITimeIntervalGEVtbl;

    interface ITimeIntervalGE
    {
        CONST_VTBL struct ITimeIntervalGEVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITimeIntervalGE_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITimeIntervalGE_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITimeIntervalGE_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITimeIntervalGE_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITimeIntervalGE_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITimeIntervalGE_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITimeIntervalGE_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITimeIntervalGE_get_BeginTime(This,pTime)	\
    (This)->lpVtbl -> get_BeginTime(This,pTime)

#define ITimeIntervalGE_get_EndTime(This,pTime)	\
    (This)->lpVtbl -> get_EndTime(This,pTime)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITimeIntervalGE_get_BeginTime_Proxy( 
    ITimeIntervalGE * This,
    /* [retval][out] */ ITimeGE **pTime);


void __RPC_STUB ITimeIntervalGE_get_BeginTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITimeIntervalGE_get_EndTime_Proxy( 
    ITimeIntervalGE * This,
    /* [retval][out] */ ITimeGE **pTime);


void __RPC_STUB ITimeIntervalGE_get_EndTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITimeIntervalGE_INTERFACE_DEFINED__ */


#ifndef __ICameraInfoGE_INTERFACE_DEFINED__
#define __ICameraInfoGE_INTERFACE_DEFINED__

/* interface ICameraInfoGE */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICameraInfoGE;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("08D46BCD-AF56-4175-999E-6DDC3771C64E")
    ICameraInfoGE : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FocusPointLatitude( 
            /* [retval][out] */ double *pLat) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_FocusPointLatitude( 
            /* [in] */ double lat) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FocusPointLongitude( 
            /* [retval][out] */ double *pLon) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_FocusPointLongitude( 
            /* [in] */ double lon) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FocusPointAltitude( 
            /* [retval][out] */ double *pAlt) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_FocusPointAltitude( 
            /* [in] */ double alt) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FocusPointAltitudeMode( 
            /* [retval][out] */ AltitudeModeGE *pAltMode) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_FocusPointAltitudeMode( 
            /* [in] */ AltitudeModeGE altMode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Range( 
            /* [retval][out] */ double *pRange) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Range( 
            /* [in] */ double range) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Tilt( 
            /* [retval][out] */ double *pTilt) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Tilt( 
            /* [in] */ double tilt) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Azimuth( 
            /* [retval][out] */ double *pAzimuth) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Azimuth( 
            /* [in] */ double azimuth) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICameraInfoGEVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICameraInfoGE * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICameraInfoGE * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICameraInfoGE * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICameraInfoGE * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICameraInfoGE * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICameraInfoGE * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICameraInfoGE * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FocusPointLatitude )( 
            ICameraInfoGE * This,
            /* [retval][out] */ double *pLat);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FocusPointLatitude )( 
            ICameraInfoGE * This,
            /* [in] */ double lat);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FocusPointLongitude )( 
            ICameraInfoGE * This,
            /* [retval][out] */ double *pLon);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FocusPointLongitude )( 
            ICameraInfoGE * This,
            /* [in] */ double lon);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FocusPointAltitude )( 
            ICameraInfoGE * This,
            /* [retval][out] */ double *pAlt);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FocusPointAltitude )( 
            ICameraInfoGE * This,
            /* [in] */ double alt);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FocusPointAltitudeMode )( 
            ICameraInfoGE * This,
            /* [retval][out] */ AltitudeModeGE *pAltMode);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FocusPointAltitudeMode )( 
            ICameraInfoGE * This,
            /* [in] */ AltitudeModeGE altMode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Range )( 
            ICameraInfoGE * This,
            /* [retval][out] */ double *pRange);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Range )( 
            ICameraInfoGE * This,
            /* [in] */ double range);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Tilt )( 
            ICameraInfoGE * This,
            /* [retval][out] */ double *pTilt);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Tilt )( 
            ICameraInfoGE * This,
            /* [in] */ double tilt);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Azimuth )( 
            ICameraInfoGE * This,
            /* [retval][out] */ double *pAzimuth);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Azimuth )( 
            ICameraInfoGE * This,
            /* [in] */ double azimuth);
        
        END_INTERFACE
    } ICameraInfoGEVtbl;

    interface ICameraInfoGE
    {
        CONST_VTBL struct ICameraInfoGEVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICameraInfoGE_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICameraInfoGE_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICameraInfoGE_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICameraInfoGE_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICameraInfoGE_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICameraInfoGE_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICameraInfoGE_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICameraInfoGE_get_FocusPointLatitude(This,pLat)	\
    (This)->lpVtbl -> get_FocusPointLatitude(This,pLat)

#define ICameraInfoGE_put_FocusPointLatitude(This,lat)	\
    (This)->lpVtbl -> put_FocusPointLatitude(This,lat)

#define ICameraInfoGE_get_FocusPointLongitude(This,pLon)	\
    (This)->lpVtbl -> get_FocusPointLongitude(This,pLon)

#define ICameraInfoGE_put_FocusPointLongitude(This,lon)	\
    (This)->lpVtbl -> put_FocusPointLongitude(This,lon)

#define ICameraInfoGE_get_FocusPointAltitude(This,pAlt)	\
    (This)->lpVtbl -> get_FocusPointAltitude(This,pAlt)

#define ICameraInfoGE_put_FocusPointAltitude(This,alt)	\
    (This)->lpVtbl -> put_FocusPointAltitude(This,alt)

#define ICameraInfoGE_get_FocusPointAltitudeMode(This,pAltMode)	\
    (This)->lpVtbl -> get_FocusPointAltitudeMode(This,pAltMode)

#define ICameraInfoGE_put_FocusPointAltitudeMode(This,altMode)	\
    (This)->lpVtbl -> put_FocusPointAltitudeMode(This,altMode)

#define ICameraInfoGE_get_Range(This,pRange)	\
    (This)->lpVtbl -> get_Range(This,pRange)

#define ICameraInfoGE_put_Range(This,range)	\
    (This)->lpVtbl -> put_Range(This,range)

#define ICameraInfoGE_get_Tilt(This,pTilt)	\
    (This)->lpVtbl -> get_Tilt(This,pTilt)

#define ICameraInfoGE_put_Tilt(This,tilt)	\
    (This)->lpVtbl -> put_Tilt(This,tilt)

#define ICameraInfoGE_get_Azimuth(This,pAzimuth)	\
    (This)->lpVtbl -> get_Azimuth(This,pAzimuth)

#define ICameraInfoGE_put_Azimuth(This,azimuth)	\
    (This)->lpVtbl -> put_Azimuth(This,azimuth)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICameraInfoGE_get_FocusPointLatitude_Proxy( 
    ICameraInfoGE * This,
    /* [retval][out] */ double *pLat);


void __RPC_STUB ICameraInfoGE_get_FocusPointLatitude_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ICameraInfoGE_put_FocusPointLatitude_Proxy( 
    ICameraInfoGE * This,
    /* [in] */ double lat);


void __RPC_STUB ICameraInfoGE_put_FocusPointLatitude_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICameraInfoGE_get_FocusPointLongitude_Proxy( 
    ICameraInfoGE * This,
    /* [retval][out] */ double *pLon);


void __RPC_STUB ICameraInfoGE_get_FocusPointLongitude_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ICameraInfoGE_put_FocusPointLongitude_Proxy( 
    ICameraInfoGE * This,
    /* [in] */ double lon);


void __RPC_STUB ICameraInfoGE_put_FocusPointLongitude_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICameraInfoGE_get_FocusPointAltitude_Proxy( 
    ICameraInfoGE * This,
    /* [retval][out] */ double *pAlt);


void __RPC_STUB ICameraInfoGE_get_FocusPointAltitude_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ICameraInfoGE_put_FocusPointAltitude_Proxy( 
    ICameraInfoGE * This,
    /* [in] */ double alt);


void __RPC_STUB ICameraInfoGE_put_FocusPointAltitude_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICameraInfoGE_get_FocusPointAltitudeMode_Proxy( 
    ICameraInfoGE * This,
    /* [retval][out] */ AltitudeModeGE *pAltMode);


void __RPC_STUB ICameraInfoGE_get_FocusPointAltitudeMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ICameraInfoGE_put_FocusPointAltitudeMode_Proxy( 
    ICameraInfoGE * This,
    /* [in] */ AltitudeModeGE altMode);


void __RPC_STUB ICameraInfoGE_put_FocusPointAltitudeMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICameraInfoGE_get_Range_Proxy( 
    ICameraInfoGE * This,
    /* [retval][out] */ double *pRange);


void __RPC_STUB ICameraInfoGE_get_Range_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ICameraInfoGE_put_Range_Proxy( 
    ICameraInfoGE * This,
    /* [in] */ double range);


void __RPC_STUB ICameraInfoGE_put_Range_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICameraInfoGE_get_Tilt_Proxy( 
    ICameraInfoGE * This,
    /* [retval][out] */ double *pTilt);


void __RPC_STUB ICameraInfoGE_get_Tilt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ICameraInfoGE_put_Tilt_Proxy( 
    ICameraInfoGE * This,
    /* [in] */ double tilt);


void __RPC_STUB ICameraInfoGE_put_Tilt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICameraInfoGE_get_Azimuth_Proxy( 
    ICameraInfoGE * This,
    /* [retval][out] */ double *pAzimuth);


void __RPC_STUB ICameraInfoGE_get_Azimuth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ICameraInfoGE_put_Azimuth_Proxy( 
    ICameraInfoGE * This,
    /* [in] */ double azimuth);


void __RPC_STUB ICameraInfoGE_put_Azimuth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICameraInfoGE_INTERFACE_DEFINED__ */


#ifndef __ITourControllerGE_INTERFACE_DEFINED__
#define __ITourControllerGE_INTERFACE_DEFINED__

/* interface ITourControllerGE */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ITourControllerGE;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D08577E0-365E-4216-B1A4-19353EAC1602")
    ITourControllerGE : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE PlayOrPause( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Speed( 
            /* [retval][out] */ double *pSpeed) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Speed( 
            /* [in] */ double speed) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PauseDelay( 
            /* [retval][out] */ double *pPauseDelay) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_PauseDelay( 
            /* [in] */ double pauseDelay) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Cycles( 
            /* [retval][out] */ int *pCycles) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Cycles( 
            /* [in] */ int cycles) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITourControllerGEVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITourControllerGE * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITourControllerGE * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITourControllerGE * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITourControllerGE * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITourControllerGE * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITourControllerGE * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITourControllerGE * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *PlayOrPause )( 
            ITourControllerGE * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Stop )( 
            ITourControllerGE * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Speed )( 
            ITourControllerGE * This,
            /* [retval][out] */ double *pSpeed);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Speed )( 
            ITourControllerGE * This,
            /* [in] */ double speed);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PauseDelay )( 
            ITourControllerGE * This,
            /* [retval][out] */ double *pPauseDelay);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PauseDelay )( 
            ITourControllerGE * This,
            /* [in] */ double pauseDelay);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Cycles )( 
            ITourControllerGE * This,
            /* [retval][out] */ int *pCycles);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Cycles )( 
            ITourControllerGE * This,
            /* [in] */ int cycles);
        
        END_INTERFACE
    } ITourControllerGEVtbl;

    interface ITourControllerGE
    {
        CONST_VTBL struct ITourControllerGEVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITourControllerGE_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITourControllerGE_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITourControllerGE_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITourControllerGE_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITourControllerGE_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITourControllerGE_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITourControllerGE_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITourControllerGE_PlayOrPause(This)	\
    (This)->lpVtbl -> PlayOrPause(This)

#define ITourControllerGE_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define ITourControllerGE_get_Speed(This,pSpeed)	\
    (This)->lpVtbl -> get_Speed(This,pSpeed)

#define ITourControllerGE_put_Speed(This,speed)	\
    (This)->lpVtbl -> put_Speed(This,speed)

#define ITourControllerGE_get_PauseDelay(This,pPauseDelay)	\
    (This)->lpVtbl -> get_PauseDelay(This,pPauseDelay)

#define ITourControllerGE_put_PauseDelay(This,pauseDelay)	\
    (This)->lpVtbl -> put_PauseDelay(This,pauseDelay)

#define ITourControllerGE_get_Cycles(This,pCycles)	\
    (This)->lpVtbl -> get_Cycles(This,pCycles)

#define ITourControllerGE_put_Cycles(This,cycles)	\
    (This)->lpVtbl -> put_Cycles(This,cycles)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITourControllerGE_PlayOrPause_Proxy( 
    ITourControllerGE * This);


void __RPC_STUB ITourControllerGE_PlayOrPause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITourControllerGE_Stop_Proxy( 
    ITourControllerGE * This);


void __RPC_STUB ITourControllerGE_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITourControllerGE_get_Speed_Proxy( 
    ITourControllerGE * This,
    /* [retval][out] */ double *pSpeed);


void __RPC_STUB ITourControllerGE_get_Speed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITourControllerGE_put_Speed_Proxy( 
    ITourControllerGE * This,
    /* [in] */ double speed);


void __RPC_STUB ITourControllerGE_put_Speed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITourControllerGE_get_PauseDelay_Proxy( 
    ITourControllerGE * This,
    /* [retval][out] */ double *pPauseDelay);


void __RPC_STUB ITourControllerGE_get_PauseDelay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITourControllerGE_put_PauseDelay_Proxy( 
    ITourControllerGE * This,
    /* [in] */ double pauseDelay);


void __RPC_STUB ITourControllerGE_put_PauseDelay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITourControllerGE_get_Cycles_Proxy( 
    ITourControllerGE * This,
    /* [retval][out] */ int *pCycles);


void __RPC_STUB ITourControllerGE_get_Cycles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITourControllerGE_put_Cycles_Proxy( 
    ITourControllerGE * This,
    /* [in] */ int cycles);


void __RPC_STUB ITourControllerGE_put_Cycles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITourControllerGE_INTERFACE_DEFINED__ */


#ifndef __ISearchControllerGE_INTERFACE_DEFINED__
#define __ISearchControllerGE_INTERFACE_DEFINED__

/* interface ISearchControllerGE */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISearchControllerGE;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("524E5B0F-D593-45A6-9F87-1BAE7D338373")
    ISearchControllerGE : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Search( 
            /* [in] */ BSTR searchString) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsSearchInProgress( 
            /* [retval][out] */ BOOL *inProgress) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetResults( 
            /* [retval][out] */ IFeatureCollectionGE **pResults) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ClearResults( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISearchControllerGEVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISearchControllerGE * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISearchControllerGE * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISearchControllerGE * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISearchControllerGE * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISearchControllerGE * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISearchControllerGE * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISearchControllerGE * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Search )( 
            ISearchControllerGE * This,
            /* [in] */ BSTR searchString);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsSearchInProgress )( 
            ISearchControllerGE * This,
            /* [retval][out] */ BOOL *inProgress);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetResults )( 
            ISearchControllerGE * This,
            /* [retval][out] */ IFeatureCollectionGE **pResults);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ClearResults )( 
            ISearchControllerGE * This);
        
        END_INTERFACE
    } ISearchControllerGEVtbl;

    interface ISearchControllerGE
    {
        CONST_VTBL struct ISearchControllerGEVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISearchControllerGE_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISearchControllerGE_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISearchControllerGE_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISearchControllerGE_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISearchControllerGE_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISearchControllerGE_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISearchControllerGE_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISearchControllerGE_Search(This,searchString)	\
    (This)->lpVtbl -> Search(This,searchString)

#define ISearchControllerGE_IsSearchInProgress(This,inProgress)	\
    (This)->lpVtbl -> IsSearchInProgress(This,inProgress)

#define ISearchControllerGE_GetResults(This,pResults)	\
    (This)->lpVtbl -> GetResults(This,pResults)

#define ISearchControllerGE_ClearResults(This)	\
    (This)->lpVtbl -> ClearResults(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISearchControllerGE_Search_Proxy( 
    ISearchControllerGE * This,
    /* [in] */ BSTR searchString);


void __RPC_STUB ISearchControllerGE_Search_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISearchControllerGE_IsSearchInProgress_Proxy( 
    ISearchControllerGE * This,
    /* [retval][out] */ BOOL *inProgress);


void __RPC_STUB ISearchControllerGE_IsSearchInProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISearchControllerGE_GetResults_Proxy( 
    ISearchControllerGE * This,
    /* [retval][out] */ IFeatureCollectionGE **pResults);


void __RPC_STUB ISearchControllerGE_GetResults_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISearchControllerGE_ClearResults_Proxy( 
    ISearchControllerGE * This);


void __RPC_STUB ISearchControllerGE_ClearResults_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISearchControllerGE_INTERFACE_DEFINED__ */


#ifndef __IAnimationControllerGE_INTERFACE_DEFINED__
#define __IAnimationControllerGE_INTERFACE_DEFINED__

/* interface IAnimationControllerGE */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IAnimationControllerGE;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BE5E5F15-8EC4-4dCC-B48D-9957D2DE4D05")
    IAnimationControllerGE : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Play( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SliderTimeInterval( 
            /* [retval][out] */ ITimeIntervalGE **pInterval) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentTimeInterval( 
            /* [retval][out] */ ITimeIntervalGE **pInterval) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_CurrentTimeInterval( 
            /* [in] */ ITimeIntervalGE *interval) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAnimationControllerGEVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAnimationControllerGE * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAnimationControllerGE * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAnimationControllerGE * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAnimationControllerGE * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAnimationControllerGE * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAnimationControllerGE * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAnimationControllerGE * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Play )( 
            IAnimationControllerGE * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Pause )( 
            IAnimationControllerGE * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SliderTimeInterval )( 
            IAnimationControllerGE * This,
            /* [retval][out] */ ITimeIntervalGE **pInterval);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentTimeInterval )( 
            IAnimationControllerGE * This,
            /* [retval][out] */ ITimeIntervalGE **pInterval);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CurrentTimeInterval )( 
            IAnimationControllerGE * This,
            /* [in] */ ITimeIntervalGE *interval);
        
        END_INTERFACE
    } IAnimationControllerGEVtbl;

    interface IAnimationControllerGE
    {
        CONST_VTBL struct IAnimationControllerGEVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAnimationControllerGE_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAnimationControllerGE_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAnimationControllerGE_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAnimationControllerGE_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAnimationControllerGE_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAnimationControllerGE_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAnimationControllerGE_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAnimationControllerGE_Play(This)	\
    (This)->lpVtbl -> Play(This)

#define IAnimationControllerGE_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define IAnimationControllerGE_get_SliderTimeInterval(This,pInterval)	\
    (This)->lpVtbl -> get_SliderTimeInterval(This,pInterval)

#define IAnimationControllerGE_get_CurrentTimeInterval(This,pInterval)	\
    (This)->lpVtbl -> get_CurrentTimeInterval(This,pInterval)

#define IAnimationControllerGE_put_CurrentTimeInterval(This,interval)	\
    (This)->lpVtbl -> put_CurrentTimeInterval(This,interval)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAnimationControllerGE_Play_Proxy( 
    IAnimationControllerGE * This);


void __RPC_STUB IAnimationControllerGE_Play_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAnimationControllerGE_Pause_Proxy( 
    IAnimationControllerGE * This);


void __RPC_STUB IAnimationControllerGE_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAnimationControllerGE_get_SliderTimeInterval_Proxy( 
    IAnimationControllerGE * This,
    /* [retval][out] */ ITimeIntervalGE **pInterval);


void __RPC_STUB IAnimationControllerGE_get_SliderTimeInterval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IAnimationControllerGE_get_CurrentTimeInterval_Proxy( 
    IAnimationControllerGE * This,
    /* [retval][out] */ ITimeIntervalGE **pInterval);


void __RPC_STUB IAnimationControllerGE_get_CurrentTimeInterval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IAnimationControllerGE_put_CurrentTimeInterval_Proxy( 
    IAnimationControllerGE * This,
    /* [in] */ ITimeIntervalGE *interval);


void __RPC_STUB IAnimationControllerGE_put_CurrentTimeInterval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAnimationControllerGE_INTERFACE_DEFINED__ */


#ifndef __IViewExtentsGE_INTERFACE_DEFINED__
#define __IViewExtentsGE_INTERFACE_DEFINED__

/* interface IViewExtentsGE */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IViewExtentsGE;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("865AB2C1-38C5-492B-8B71-AC73F5A7A43D")
    IViewExtentsGE : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_North( 
            /* [retval][out] */ double *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_South( 
            /* [retval][out] */ double *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_East( 
            /* [retval][out] */ double *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_West( 
            /* [retval][out] */ double *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IViewExtentsGEVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IViewExtentsGE * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IViewExtentsGE * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IViewExtentsGE * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IViewExtentsGE * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IViewExtentsGE * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IViewExtentsGE * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IViewExtentsGE * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_North )( 
            IViewExtentsGE * This,
            /* [retval][out] */ double *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_South )( 
            IViewExtentsGE * This,
            /* [retval][out] */ double *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_East )( 
            IViewExtentsGE * This,
            /* [retval][out] */ double *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_West )( 
            IViewExtentsGE * This,
            /* [retval][out] */ double *pVal);
        
        END_INTERFACE
    } IViewExtentsGEVtbl;

    interface IViewExtentsGE
    {
        CONST_VTBL struct IViewExtentsGEVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IViewExtentsGE_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IViewExtentsGE_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IViewExtentsGE_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IViewExtentsGE_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IViewExtentsGE_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IViewExtentsGE_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IViewExtentsGE_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IViewExtentsGE_get_North(This,pVal)	\
    (This)->lpVtbl -> get_North(This,pVal)

#define IViewExtentsGE_get_South(This,pVal)	\
    (This)->lpVtbl -> get_South(This,pVal)

#define IViewExtentsGE_get_East(This,pVal)	\
    (This)->lpVtbl -> get_East(This,pVal)

#define IViewExtentsGE_get_West(This,pVal)	\
    (This)->lpVtbl -> get_West(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IViewExtentsGE_get_North_Proxy( 
    IViewExtentsGE * This,
    /* [retval][out] */ double *pVal);


void __RPC_STUB IViewExtentsGE_get_North_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IViewExtentsGE_get_South_Proxy( 
    IViewExtentsGE * This,
    /* [retval][out] */ double *pVal);


void __RPC_STUB IViewExtentsGE_get_South_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IViewExtentsGE_get_East_Proxy( 
    IViewExtentsGE * This,
    /* [retval][out] */ double *pVal);


void __RPC_STUB IViewExtentsGE_get_East_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IViewExtentsGE_get_West_Proxy( 
    IViewExtentsGE * This,
    /* [retval][out] */ double *pVal);


void __RPC_STUB IViewExtentsGE_get_West_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IViewExtentsGE_INTERFACE_DEFINED__ */


#ifndef __IFeatureGE_INTERFACE_DEFINED__
#define __IFeatureGE_INTERFACE_DEFINED__

/* interface IFeatureGE */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFeatureGE;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("92547B06-0007-4820-B76A-C84E402CA709")
    IFeatureGE : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *name) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Visibility( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Visibility( 
            /* [in] */ BOOL newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_HasView( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Highlighted( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Highlight( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetParent( 
            /* [retval][out] */ IFeatureGE **pParent) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetChildren( 
            /* [retval][out] */ IFeatureCollectionGE **pChildren) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TimeInterval( 
            /* [retval][out] */ ITimeIntervalGE **pInterval) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFeatureGEVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFeatureGE * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFeatureGE * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFeatureGE * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFeatureGE * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFeatureGE * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFeatureGE * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFeatureGE * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IFeatureGE * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Visibility )( 
            IFeatureGE * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Visibility )( 
            IFeatureGE * This,
            /* [in] */ BOOL newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HasView )( 
            IFeatureGE * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Highlighted )( 
            IFeatureGE * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Highlight )( 
            IFeatureGE * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IFeatureGE * This,
            /* [retval][out] */ IFeatureGE **pParent);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetChildren )( 
            IFeatureGE * This,
            /* [retval][out] */ IFeatureCollectionGE **pChildren);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TimeInterval )( 
            IFeatureGE * This,
            /* [retval][out] */ ITimeIntervalGE **pInterval);
        
        END_INTERFACE
    } IFeatureGEVtbl;

    interface IFeatureGE
    {
        CONST_VTBL struct IFeatureGEVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFeatureGE_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFeatureGE_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFeatureGE_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFeatureGE_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFeatureGE_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFeatureGE_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFeatureGE_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFeatureGE_get_Name(This,name)	\
    (This)->lpVtbl -> get_Name(This,name)

#define IFeatureGE_get_Visibility(This,pVal)	\
    (This)->lpVtbl -> get_Visibility(This,pVal)

#define IFeatureGE_put_Visibility(This,newVal)	\
    (This)->lpVtbl -> put_Visibility(This,newVal)

#define IFeatureGE_get_HasView(This,pVal)	\
    (This)->lpVtbl -> get_HasView(This,pVal)

#define IFeatureGE_get_Highlighted(This,pVal)	\
    (This)->lpVtbl -> get_Highlighted(This,pVal)

#define IFeatureGE_Highlight(This)	\
    (This)->lpVtbl -> Highlight(This)

#define IFeatureGE_GetParent(This,pParent)	\
    (This)->lpVtbl -> GetParent(This,pParent)

#define IFeatureGE_GetChildren(This,pChildren)	\
    (This)->lpVtbl -> GetChildren(This,pChildren)

#define IFeatureGE_get_TimeInterval(This,pInterval)	\
    (This)->lpVtbl -> get_TimeInterval(This,pInterval)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFeatureGE_get_Name_Proxy( 
    IFeatureGE * This,
    /* [retval][out] */ BSTR *name);


void __RPC_STUB IFeatureGE_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFeatureGE_get_Visibility_Proxy( 
    IFeatureGE * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IFeatureGE_get_Visibility_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFeatureGE_put_Visibility_Proxy( 
    IFeatureGE * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB IFeatureGE_put_Visibility_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFeatureGE_get_HasView_Proxy( 
    IFeatureGE * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IFeatureGE_get_HasView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFeatureGE_get_Highlighted_Proxy( 
    IFeatureGE * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IFeatureGE_get_Highlighted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFeatureGE_Highlight_Proxy( 
    IFeatureGE * This);


void __RPC_STUB IFeatureGE_Highlight_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFeatureGE_GetParent_Proxy( 
    IFeatureGE * This,
    /* [retval][out] */ IFeatureGE **pParent);


void __RPC_STUB IFeatureGE_GetParent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFeatureGE_GetChildren_Proxy( 
    IFeatureGE * This,
    /* [retval][out] */ IFeatureCollectionGE **pChildren);


void __RPC_STUB IFeatureGE_GetChildren_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFeatureGE_get_TimeInterval_Proxy( 
    IFeatureGE * This,
    /* [retval][out] */ ITimeIntervalGE **pInterval);


void __RPC_STUB IFeatureGE_get_TimeInterval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFeatureGE_INTERFACE_DEFINED__ */


#ifndef __IFeatureCollectionGE_INTERFACE_DEFINED__
#define __IFeatureCollectionGE_INTERFACE_DEFINED__

/* interface IFeatureCollectionGE */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFeatureCollectionGE;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("851D25E7-785F-4DB7-95F9-A0EF7E836C44")
    IFeatureCollectionGE : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppUnk) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long index,
            /* [retval][out] */ IFeatureGE **pFeature) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *count) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFeatureCollectionGEVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFeatureCollectionGE * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFeatureCollectionGE * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFeatureCollectionGE * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFeatureCollectionGE * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFeatureCollectionGE * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFeatureCollectionGE * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFeatureCollectionGE * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IFeatureCollectionGE * This,
            /* [retval][out] */ IUnknown **ppUnk);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IFeatureCollectionGE * This,
            /* [in] */ long index,
            /* [retval][out] */ IFeatureGE **pFeature);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IFeatureCollectionGE * This,
            /* [retval][out] */ long *count);
        
        END_INTERFACE
    } IFeatureCollectionGEVtbl;

    interface IFeatureCollectionGE
    {
        CONST_VTBL struct IFeatureCollectionGEVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFeatureCollectionGE_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFeatureCollectionGE_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFeatureCollectionGE_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFeatureCollectionGE_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFeatureCollectionGE_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFeatureCollectionGE_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFeatureCollectionGE_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFeatureCollectionGE_get__NewEnum(This,ppUnk)	\
    (This)->lpVtbl -> get__NewEnum(This,ppUnk)

#define IFeatureCollectionGE_get_Item(This,index,pFeature)	\
    (This)->lpVtbl -> get_Item(This,index,pFeature)

#define IFeatureCollectionGE_get_Count(This,count)	\
    (This)->lpVtbl -> get_Count(This,count)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFeatureCollectionGE_get__NewEnum_Proxy( 
    IFeatureCollectionGE * This,
    /* [retval][out] */ IUnknown **ppUnk);


void __RPC_STUB IFeatureCollectionGE_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFeatureCollectionGE_get_Item_Proxy( 
    IFeatureCollectionGE * This,
    /* [in] */ long index,
    /* [retval][out] */ IFeatureGE **pFeature);


void __RPC_STUB IFeatureCollectionGE_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFeatureCollectionGE_get_Count_Proxy( 
    IFeatureCollectionGE * This,
    /* [retval][out] */ long *count);


void __RPC_STUB IFeatureCollectionGE_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFeatureCollectionGE_INTERFACE_DEFINED__ */


#ifndef __IPointOnTerrainGE_INTERFACE_DEFINED__
#define __IPointOnTerrainGE_INTERFACE_DEFINED__

/* interface IPointOnTerrainGE */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IPointOnTerrainGE;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F4F7B301-7C59-4851-BA97-C51F110B590F")
    IPointOnTerrainGE : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Latitude( 
            /* [retval][out] */ double *pLat) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Longitude( 
            /* [retval][out] */ double *pLon) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Altitude( 
            /* [retval][out] */ double *pAlt) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ProjectedOntoGlobe( 
            /* [retval][out] */ BOOL *pProjected) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ZeroElevationExaggeration( 
            /* [retval][out] */ BOOL *pZeroExag) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPointOnTerrainGEVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPointOnTerrainGE * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPointOnTerrainGE * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPointOnTerrainGE * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPointOnTerrainGE * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPointOnTerrainGE * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPointOnTerrainGE * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPointOnTerrainGE * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Latitude )( 
            IPointOnTerrainGE * This,
            /* [retval][out] */ double *pLat);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Longitude )( 
            IPointOnTerrainGE * This,
            /* [retval][out] */ double *pLon);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Altitude )( 
            IPointOnTerrainGE * This,
            /* [retval][out] */ double *pAlt);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProjectedOntoGlobe )( 
            IPointOnTerrainGE * This,
            /* [retval][out] */ BOOL *pProjected);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ZeroElevationExaggeration )( 
            IPointOnTerrainGE * This,
            /* [retval][out] */ BOOL *pZeroExag);
        
        END_INTERFACE
    } IPointOnTerrainGEVtbl;

    interface IPointOnTerrainGE
    {
        CONST_VTBL struct IPointOnTerrainGEVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPointOnTerrainGE_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPointOnTerrainGE_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPointOnTerrainGE_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPointOnTerrainGE_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IPointOnTerrainGE_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IPointOnTerrainGE_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IPointOnTerrainGE_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IPointOnTerrainGE_get_Latitude(This,pLat)	\
    (This)->lpVtbl -> get_Latitude(This,pLat)

#define IPointOnTerrainGE_get_Longitude(This,pLon)	\
    (This)->lpVtbl -> get_Longitude(This,pLon)

#define IPointOnTerrainGE_get_Altitude(This,pAlt)	\
    (This)->lpVtbl -> get_Altitude(This,pAlt)

#define IPointOnTerrainGE_get_ProjectedOntoGlobe(This,pProjected)	\
    (This)->lpVtbl -> get_ProjectedOntoGlobe(This,pProjected)

#define IPointOnTerrainGE_get_ZeroElevationExaggeration(This,pZeroExag)	\
    (This)->lpVtbl -> get_ZeroElevationExaggeration(This,pZeroExag)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPointOnTerrainGE_get_Latitude_Proxy( 
    IPointOnTerrainGE * This,
    /* [retval][out] */ double *pLat);


void __RPC_STUB IPointOnTerrainGE_get_Latitude_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPointOnTerrainGE_get_Longitude_Proxy( 
    IPointOnTerrainGE * This,
    /* [retval][out] */ double *pLon);


void __RPC_STUB IPointOnTerrainGE_get_Longitude_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPointOnTerrainGE_get_Altitude_Proxy( 
    IPointOnTerrainGE * This,
    /* [retval][out] */ double *pAlt);


void __RPC_STUB IPointOnTerrainGE_get_Altitude_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPointOnTerrainGE_get_ProjectedOntoGlobe_Proxy( 
    IPointOnTerrainGE * This,
    /* [retval][out] */ BOOL *pProjected);


void __RPC_STUB IPointOnTerrainGE_get_ProjectedOntoGlobe_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IPointOnTerrainGE_get_ZeroElevationExaggeration_Proxy( 
    IPointOnTerrainGE * This,
    /* [retval][out] */ BOOL *pZeroExag);


void __RPC_STUB IPointOnTerrainGE_get_ZeroElevationExaggeration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPointOnTerrainGE_INTERFACE_DEFINED__ */


#ifndef __IApplicationGE_INTERFACE_DEFINED__
#define __IApplicationGE_INTERFACE_DEFINED__

/* interface IApplicationGE */
/* [unique][helpstring][dual][uuid][object] */ 

typedef /* [public][public][v1_enum] */ 
enum __MIDL_IApplicationGE_0001
    {	EnterpriseClientGE	= 0,
	ProGE	= 1,
	PlusGE	= 2,
	FreeGE	= 5,
	UnknownGE	= 0xff
    } 	AppTypeGE;


EXTERN_C const IID IID_IApplicationGE;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2830837B-D4E8-48C6-B6EE-04633372ABE4")
    IApplicationGE : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetCamera( 
            /* [in] */ BOOL considerTerrain,
            /* [retval][out] */ ICameraInfoGE **pCamera) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetCamera( 
            /* [in] */ ICameraInfoGE *camera,
            /* [in] */ double speed) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetCameraParams( 
            /* [in] */ double lat,
            /* [in] */ double lon,
            /* [in] */ double alt,
            /* [in] */ AltitudeModeGE altMode,
            /* [in] */ double range,
            /* [in] */ double tilt,
            /* [in] */ double azimuth,
            /* [in] */ double speed) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_StreamingProgressPercentage( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SaveScreenShot( 
            /* [in] */ BSTR fileName,
            /* [in] */ long quality) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OpenKmlFile( 
            /* [in] */ BSTR fileName,
            /* [in] */ BOOL suppressMessages) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE LoadKmlData( 
            /* [in] */ BSTR *kmlData) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AutoPilotSpeed( 
            /* [retval][out] */ double *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AutoPilotSpeed( 
            /* [in] */ double newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ViewExtents( 
            /* [retval][out] */ IViewExtentsGE **pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetFeatureByName( 
            /* [in] */ BSTR name,
            /* [retval][out] */ IFeatureGE **pFeature) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetFeatureByHref( 
            /* [in] */ BSTR href,
            /* [retval][out] */ IFeatureGE **pFeature) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetFeatureView( 
            /* [in] */ IFeatureGE *feature,
            /* [in] */ double speed) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetPointOnTerrainFromScreenCoords( 
            /* [in] */ double screen_x,
            /* [in] */ double screen_y,
            /* [retval][out] */ IPointOnTerrainGE **pPoint) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_VersionMajor( 
            /* [retval][out] */ int *major) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_VersionMinor( 
            /* [retval][out] */ int *minor) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_VersionBuild( 
            /* [retval][out] */ int *build) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_VersionAppType( 
            /* [retval][out] */ AppTypeGE *appType) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsInitialized( 
            /* [retval][out] */ BOOL *isInitialized) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsOnline( 
            /* [retval][out] */ BOOL *isOnline) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Login( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Logout( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ShowDescriptionBalloon( 
            /* [in] */ IFeatureGE *feature) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE HideDescriptionBalloons( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetHighlightedFeature( 
            /* [retval][out] */ IFeatureGE **pFeature) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetMyPlaces( 
            /* [retval][out] */ IFeatureGE **pMyPlaces) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetTemporaryPlaces( 
            /* [retval][out] */ IFeatureGE **pTemporaryPlaces) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetLayersDatabases( 
            /* [retval][out] */ IFeatureCollectionGE **pDatabases) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ElevationExaggeration( 
            /* [retval][out] */ double *pExaggeration) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ElevationExaggeration( 
            /* [in] */ double exaggeration) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetMainHwnd( 
            /* [retval][out] */ OLE_HANDLE *hwnd) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TourController( 
            /* [retval][out] */ ITourControllerGE **pTourController) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SearchController( 
            /* [retval][out] */ ISearchControllerGE **pSearchController) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AnimationController( 
            /* [retval][out] */ IAnimationControllerGE **pAnimationController) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetRenderHwnd( 
            /* [retval][out] */ OLE_HANDLE *hwnd) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IApplicationGEVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IApplicationGE * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IApplicationGE * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IApplicationGE * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IApplicationGE * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IApplicationGE * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IApplicationGE * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IApplicationGE * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetCamera )( 
            IApplicationGE * This,
            /* [in] */ BOOL considerTerrain,
            /* [retval][out] */ ICameraInfoGE **pCamera);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetCamera )( 
            IApplicationGE * This,
            /* [in] */ ICameraInfoGE *camera,
            /* [in] */ double speed);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetCameraParams )( 
            IApplicationGE * This,
            /* [in] */ double lat,
            /* [in] */ double lon,
            /* [in] */ double alt,
            /* [in] */ AltitudeModeGE altMode,
            /* [in] */ double range,
            /* [in] */ double tilt,
            /* [in] */ double azimuth,
            /* [in] */ double speed);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StreamingProgressPercentage )( 
            IApplicationGE * This,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SaveScreenShot )( 
            IApplicationGE * This,
            /* [in] */ BSTR fileName,
            /* [in] */ long quality);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *OpenKmlFile )( 
            IApplicationGE * This,
            /* [in] */ BSTR fileName,
            /* [in] */ BOOL suppressMessages);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LoadKmlData )( 
            IApplicationGE * This,
            /* [in] */ BSTR *kmlData);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AutoPilotSpeed )( 
            IApplicationGE * This,
            /* [retval][out] */ double *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AutoPilotSpeed )( 
            IApplicationGE * This,
            /* [in] */ double newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ViewExtents )( 
            IApplicationGE * This,
            /* [retval][out] */ IViewExtentsGE **pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetFeatureByName )( 
            IApplicationGE * This,
            /* [in] */ BSTR name,
            /* [retval][out] */ IFeatureGE **pFeature);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetFeatureByHref )( 
            IApplicationGE * This,
            /* [in] */ BSTR href,
            /* [retval][out] */ IFeatureGE **pFeature);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetFeatureView )( 
            IApplicationGE * This,
            /* [in] */ IFeatureGE *feature,
            /* [in] */ double speed);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetPointOnTerrainFromScreenCoords )( 
            IApplicationGE * This,
            /* [in] */ double screen_x,
            /* [in] */ double screen_y,
            /* [retval][out] */ IPointOnTerrainGE **pPoint);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VersionMajor )( 
            IApplicationGE * This,
            /* [retval][out] */ int *major);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VersionMinor )( 
            IApplicationGE * This,
            /* [retval][out] */ int *minor);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VersionBuild )( 
            IApplicationGE * This,
            /* [retval][out] */ int *build);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VersionAppType )( 
            IApplicationGE * This,
            /* [retval][out] */ AppTypeGE *appType);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsInitialized )( 
            IApplicationGE * This,
            /* [retval][out] */ BOOL *isInitialized);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsOnline )( 
            IApplicationGE * This,
            /* [retval][out] */ BOOL *isOnline);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Login )( 
            IApplicationGE * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Logout )( 
            IApplicationGE * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ShowDescriptionBalloon )( 
            IApplicationGE * This,
            /* [in] */ IFeatureGE *feature);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *HideDescriptionBalloons )( 
            IApplicationGE * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetHighlightedFeature )( 
            IApplicationGE * This,
            /* [retval][out] */ IFeatureGE **pFeature);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetMyPlaces )( 
            IApplicationGE * This,
            /* [retval][out] */ IFeatureGE **pMyPlaces);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetTemporaryPlaces )( 
            IApplicationGE * This,
            /* [retval][out] */ IFeatureGE **pTemporaryPlaces);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetLayersDatabases )( 
            IApplicationGE * This,
            /* [retval][out] */ IFeatureCollectionGE **pDatabases);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ElevationExaggeration )( 
            IApplicationGE * This,
            /* [retval][out] */ double *pExaggeration);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ElevationExaggeration )( 
            IApplicationGE * This,
            /* [in] */ double exaggeration);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetMainHwnd )( 
            IApplicationGE * This,
            /* [retval][out] */ OLE_HANDLE *hwnd);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TourController )( 
            IApplicationGE * This,
            /* [retval][out] */ ITourControllerGE **pTourController);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SearchController )( 
            IApplicationGE * This,
            /* [retval][out] */ ISearchControllerGE **pSearchController);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AnimationController )( 
            IApplicationGE * This,
            /* [retval][out] */ IAnimationControllerGE **pAnimationController);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetRenderHwnd )( 
            IApplicationGE * This,
            /* [retval][out] */ OLE_HANDLE *hwnd);
        
        END_INTERFACE
    } IApplicationGEVtbl;

    interface IApplicationGE
    {
        CONST_VTBL struct IApplicationGEVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IApplicationGE_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IApplicationGE_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IApplicationGE_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IApplicationGE_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IApplicationGE_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IApplicationGE_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IApplicationGE_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IApplicationGE_GetCamera(This,considerTerrain,pCamera)	\
    (This)->lpVtbl -> GetCamera(This,considerTerrain,pCamera)

#define IApplicationGE_SetCamera(This,camera,speed)	\
    (This)->lpVtbl -> SetCamera(This,camera,speed)

#define IApplicationGE_SetCameraParams(This,lat,lon,alt,altMode,range,tilt,azimuth,speed)	\
    (This)->lpVtbl -> SetCameraParams(This,lat,lon,alt,altMode,range,tilt,azimuth,speed)

#define IApplicationGE_get_StreamingProgressPercentage(This,pVal)	\
    (This)->lpVtbl -> get_StreamingProgressPercentage(This,pVal)

#define IApplicationGE_SaveScreenShot(This,fileName,quality)	\
    (This)->lpVtbl -> SaveScreenShot(This,fileName,quality)

#define IApplicationGE_OpenKmlFile(This,fileName,suppressMessages)	\
    (This)->lpVtbl -> OpenKmlFile(This,fileName,suppressMessages)

#define IApplicationGE_LoadKmlData(This,kmlData)	\
    (This)->lpVtbl -> LoadKmlData(This,kmlData)

#define IApplicationGE_get_AutoPilotSpeed(This,pVal)	\
    (This)->lpVtbl -> get_AutoPilotSpeed(This,pVal)

#define IApplicationGE_put_AutoPilotSpeed(This,newVal)	\
    (This)->lpVtbl -> put_AutoPilotSpeed(This,newVal)

#define IApplicationGE_get_ViewExtents(This,pVal)	\
    (This)->lpVtbl -> get_ViewExtents(This,pVal)

#define IApplicationGE_GetFeatureByName(This,name,pFeature)	\
    (This)->lpVtbl -> GetFeatureByName(This,name,pFeature)

#define IApplicationGE_GetFeatureByHref(This,href,pFeature)	\
    (This)->lpVtbl -> GetFeatureByHref(This,href,pFeature)

#define IApplicationGE_SetFeatureView(This,feature,speed)	\
    (This)->lpVtbl -> SetFeatureView(This,feature,speed)

#define IApplicationGE_GetPointOnTerrainFromScreenCoords(This,screen_x,screen_y,pPoint)	\
    (This)->lpVtbl -> GetPointOnTerrainFromScreenCoords(This,screen_x,screen_y,pPoint)

#define IApplicationGE_get_VersionMajor(This,major)	\
    (This)->lpVtbl -> get_VersionMajor(This,major)

#define IApplicationGE_get_VersionMinor(This,minor)	\
    (This)->lpVtbl -> get_VersionMinor(This,minor)

#define IApplicationGE_get_VersionBuild(This,build)	\
    (This)->lpVtbl -> get_VersionBuild(This,build)

#define IApplicationGE_get_VersionAppType(This,appType)	\
    (This)->lpVtbl -> get_VersionAppType(This,appType)

#define IApplicationGE_IsInitialized(This,isInitialized)	\
    (This)->lpVtbl -> IsInitialized(This,isInitialized)

#define IApplicationGE_IsOnline(This,isOnline)	\
    (This)->lpVtbl -> IsOnline(This,isOnline)

#define IApplicationGE_Login(This)	\
    (This)->lpVtbl -> Login(This)

#define IApplicationGE_Logout(This)	\
    (This)->lpVtbl -> Logout(This)

#define IApplicationGE_ShowDescriptionBalloon(This,feature)	\
    (This)->lpVtbl -> ShowDescriptionBalloon(This,feature)

#define IApplicationGE_HideDescriptionBalloons(This)	\
    (This)->lpVtbl -> HideDescriptionBalloons(This)

#define IApplicationGE_GetHighlightedFeature(This,pFeature)	\
    (This)->lpVtbl -> GetHighlightedFeature(This,pFeature)

#define IApplicationGE_GetMyPlaces(This,pMyPlaces)	\
    (This)->lpVtbl -> GetMyPlaces(This,pMyPlaces)

#define IApplicationGE_GetTemporaryPlaces(This,pTemporaryPlaces)	\
    (This)->lpVtbl -> GetTemporaryPlaces(This,pTemporaryPlaces)

#define IApplicationGE_GetLayersDatabases(This,pDatabases)	\
    (This)->lpVtbl -> GetLayersDatabases(This,pDatabases)

#define IApplicationGE_get_ElevationExaggeration(This,pExaggeration)	\
    (This)->lpVtbl -> get_ElevationExaggeration(This,pExaggeration)

#define IApplicationGE_put_ElevationExaggeration(This,exaggeration)	\
    (This)->lpVtbl -> put_ElevationExaggeration(This,exaggeration)

#define IApplicationGE_GetMainHwnd(This,hwnd)	\
    (This)->lpVtbl -> GetMainHwnd(This,hwnd)

#define IApplicationGE_get_TourController(This,pTourController)	\
    (This)->lpVtbl -> get_TourController(This,pTourController)

#define IApplicationGE_get_SearchController(This,pSearchController)	\
    (This)->lpVtbl -> get_SearchController(This,pSearchController)

#define IApplicationGE_get_AnimationController(This,pAnimationController)	\
    (This)->lpVtbl -> get_AnimationController(This,pAnimationController)

#define IApplicationGE_GetRenderHwnd(This,hwnd)	\
    (This)->lpVtbl -> GetRenderHwnd(This,hwnd)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_GetCamera_Proxy( 
    IApplicationGE * This,
    /* [in] */ BOOL considerTerrain,
    /* [retval][out] */ ICameraInfoGE **pCamera);


void __RPC_STUB IApplicationGE_GetCamera_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_SetCamera_Proxy( 
    IApplicationGE * This,
    /* [in] */ ICameraInfoGE *camera,
    /* [in] */ double speed);


void __RPC_STUB IApplicationGE_SetCamera_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_SetCameraParams_Proxy( 
    IApplicationGE * This,
    /* [in] */ double lat,
    /* [in] */ double lon,
    /* [in] */ double alt,
    /* [in] */ AltitudeModeGE altMode,
    /* [in] */ double range,
    /* [in] */ double tilt,
    /* [in] */ double azimuth,
    /* [in] */ double speed);


void __RPC_STUB IApplicationGE_SetCameraParams_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IApplicationGE_get_StreamingProgressPercentage_Proxy( 
    IApplicationGE * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IApplicationGE_get_StreamingProgressPercentage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_SaveScreenShot_Proxy( 
    IApplicationGE * This,
    /* [in] */ BSTR fileName,
    /* [in] */ long quality);


void __RPC_STUB IApplicationGE_SaveScreenShot_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_OpenKmlFile_Proxy( 
    IApplicationGE * This,
    /* [in] */ BSTR fileName,
    /* [in] */ BOOL suppressMessages);


void __RPC_STUB IApplicationGE_OpenKmlFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_LoadKmlData_Proxy( 
    IApplicationGE * This,
    /* [in] */ BSTR *kmlData);


void __RPC_STUB IApplicationGE_LoadKmlData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IApplicationGE_get_AutoPilotSpeed_Proxy( 
    IApplicationGE * This,
    /* [retval][out] */ double *pVal);


void __RPC_STUB IApplicationGE_get_AutoPilotSpeed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IApplicationGE_put_AutoPilotSpeed_Proxy( 
    IApplicationGE * This,
    /* [in] */ double newVal);


void __RPC_STUB IApplicationGE_put_AutoPilotSpeed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IApplicationGE_get_ViewExtents_Proxy( 
    IApplicationGE * This,
    /* [retval][out] */ IViewExtentsGE **pVal);


void __RPC_STUB IApplicationGE_get_ViewExtents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_GetFeatureByName_Proxy( 
    IApplicationGE * This,
    /* [in] */ BSTR name,
    /* [retval][out] */ IFeatureGE **pFeature);


void __RPC_STUB IApplicationGE_GetFeatureByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_GetFeatureByHref_Proxy( 
    IApplicationGE * This,
    /* [in] */ BSTR href,
    /* [retval][out] */ IFeatureGE **pFeature);


void __RPC_STUB IApplicationGE_GetFeatureByHref_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_SetFeatureView_Proxy( 
    IApplicationGE * This,
    /* [in] */ IFeatureGE *feature,
    /* [in] */ double speed);


void __RPC_STUB IApplicationGE_SetFeatureView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_GetPointOnTerrainFromScreenCoords_Proxy( 
    IApplicationGE * This,
    /* [in] */ double screen_x,
    /* [in] */ double screen_y,
    /* [retval][out] */ IPointOnTerrainGE **pPoint);


void __RPC_STUB IApplicationGE_GetPointOnTerrainFromScreenCoords_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IApplicationGE_get_VersionMajor_Proxy( 
    IApplicationGE * This,
    /* [retval][out] */ int *major);


void __RPC_STUB IApplicationGE_get_VersionMajor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IApplicationGE_get_VersionMinor_Proxy( 
    IApplicationGE * This,
    /* [retval][out] */ int *minor);


void __RPC_STUB IApplicationGE_get_VersionMinor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IApplicationGE_get_VersionBuild_Proxy( 
    IApplicationGE * This,
    /* [retval][out] */ int *build);


void __RPC_STUB IApplicationGE_get_VersionBuild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IApplicationGE_get_VersionAppType_Proxy( 
    IApplicationGE * This,
    /* [retval][out] */ AppTypeGE *appType);


void __RPC_STUB IApplicationGE_get_VersionAppType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_IsInitialized_Proxy( 
    IApplicationGE * This,
    /* [retval][out] */ BOOL *isInitialized);


void __RPC_STUB IApplicationGE_IsInitialized_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_IsOnline_Proxy( 
    IApplicationGE * This,
    /* [retval][out] */ BOOL *isOnline);


void __RPC_STUB IApplicationGE_IsOnline_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_Login_Proxy( 
    IApplicationGE * This);


void __RPC_STUB IApplicationGE_Login_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_Logout_Proxy( 
    IApplicationGE * This);


void __RPC_STUB IApplicationGE_Logout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_ShowDescriptionBalloon_Proxy( 
    IApplicationGE * This,
    /* [in] */ IFeatureGE *feature);


void __RPC_STUB IApplicationGE_ShowDescriptionBalloon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_HideDescriptionBalloons_Proxy( 
    IApplicationGE * This);


void __RPC_STUB IApplicationGE_HideDescriptionBalloons_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_GetHighlightedFeature_Proxy( 
    IApplicationGE * This,
    /* [retval][out] */ IFeatureGE **pFeature);


void __RPC_STUB IApplicationGE_GetHighlightedFeature_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_GetMyPlaces_Proxy( 
    IApplicationGE * This,
    /* [retval][out] */ IFeatureGE **pMyPlaces);


void __RPC_STUB IApplicationGE_GetMyPlaces_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_GetTemporaryPlaces_Proxy( 
    IApplicationGE * This,
    /* [retval][out] */ IFeatureGE **pTemporaryPlaces);


void __RPC_STUB IApplicationGE_GetTemporaryPlaces_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_GetLayersDatabases_Proxy( 
    IApplicationGE * This,
    /* [retval][out] */ IFeatureCollectionGE **pDatabases);


void __RPC_STUB IApplicationGE_GetLayersDatabases_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IApplicationGE_get_ElevationExaggeration_Proxy( 
    IApplicationGE * This,
    /* [retval][out] */ double *pExaggeration);


void __RPC_STUB IApplicationGE_get_ElevationExaggeration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IApplicationGE_put_ElevationExaggeration_Proxy( 
    IApplicationGE * This,
    /* [in] */ double exaggeration);


void __RPC_STUB IApplicationGE_put_ElevationExaggeration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_GetMainHwnd_Proxy( 
    IApplicationGE * This,
    /* [retval][out] */ OLE_HANDLE *hwnd);


void __RPC_STUB IApplicationGE_GetMainHwnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IApplicationGE_get_TourController_Proxy( 
    IApplicationGE * This,
    /* [retval][out] */ ITourControllerGE **pTourController);


void __RPC_STUB IApplicationGE_get_TourController_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IApplicationGE_get_SearchController_Proxy( 
    IApplicationGE * This,
    /* [retval][out] */ ISearchControllerGE **pSearchController);


void __RPC_STUB IApplicationGE_get_SearchController_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IApplicationGE_get_AnimationController_Proxy( 
    IApplicationGE * This,
    /* [retval][out] */ IAnimationControllerGE **pAnimationController);


void __RPC_STUB IApplicationGE_get_AnimationController_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IApplicationGE_GetRenderHwnd_Proxy( 
    IApplicationGE * This,
    /* [retval][out] */ OLE_HANDLE *hwnd);


void __RPC_STUB IApplicationGE_GetRenderHwnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IApplicationGE_INTERFACE_DEFINED__ */


#ifndef __IKHViewInfo_INTERFACE_DEFINED__
#define __IKHViewInfo_INTERFACE_DEFINED__

/* interface IKHViewInfo */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IKHViewInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("45F89E39-7A46-4CA4-97E3-8C5AA252531C")
    IKHViewInfo : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_latitude( 
            /* [retval][out] */ double *pLat) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_latitude( 
            /* [in] */ double lat) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_longitude( 
            /* [retval][out] */ double *pLon) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_longitude( 
            /* [in] */ double lon) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_range( 
            /* [retval][out] */ double *pRange) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_range( 
            /* [in] */ double range) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_tilt( 
            /* [retval][out] */ double *pTilt) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_tilt( 
            /* [in] */ double tilt) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_azimuth( 
            /* [retval][out] */ double *pAzimuth) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_azimuth( 
            /* [in] */ double azimuth) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IKHViewInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IKHViewInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IKHViewInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IKHViewInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IKHViewInfo * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IKHViewInfo * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IKHViewInfo * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IKHViewInfo * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_latitude )( 
            IKHViewInfo * This,
            /* [retval][out] */ double *pLat);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_latitude )( 
            IKHViewInfo * This,
            /* [in] */ double lat);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_longitude )( 
            IKHViewInfo * This,
            /* [retval][out] */ double *pLon);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_longitude )( 
            IKHViewInfo * This,
            /* [in] */ double lon);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_range )( 
            IKHViewInfo * This,
            /* [retval][out] */ double *pRange);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_range )( 
            IKHViewInfo * This,
            /* [in] */ double range);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_tilt )( 
            IKHViewInfo * This,
            /* [retval][out] */ double *pTilt);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_tilt )( 
            IKHViewInfo * This,
            /* [in] */ double tilt);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_azimuth )( 
            IKHViewInfo * This,
            /* [retval][out] */ double *pAzimuth);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_azimuth )( 
            IKHViewInfo * This,
            /* [in] */ double azimuth);
        
        END_INTERFACE
    } IKHViewInfoVtbl;

    interface IKHViewInfo
    {
        CONST_VTBL struct IKHViewInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IKHViewInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IKHViewInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IKHViewInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IKHViewInfo_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IKHViewInfo_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IKHViewInfo_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IKHViewInfo_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IKHViewInfo_get_latitude(This,pLat)	\
    (This)->lpVtbl -> get_latitude(This,pLat)

#define IKHViewInfo_put_latitude(This,lat)	\
    (This)->lpVtbl -> put_latitude(This,lat)

#define IKHViewInfo_get_longitude(This,pLon)	\
    (This)->lpVtbl -> get_longitude(This,pLon)

#define IKHViewInfo_put_longitude(This,lon)	\
    (This)->lpVtbl -> put_longitude(This,lon)

#define IKHViewInfo_get_range(This,pRange)	\
    (This)->lpVtbl -> get_range(This,pRange)

#define IKHViewInfo_put_range(This,range)	\
    (This)->lpVtbl -> put_range(This,range)

#define IKHViewInfo_get_tilt(This,pTilt)	\
    (This)->lpVtbl -> get_tilt(This,pTilt)

#define IKHViewInfo_put_tilt(This,tilt)	\
    (This)->lpVtbl -> put_tilt(This,tilt)

#define IKHViewInfo_get_azimuth(This,pAzimuth)	\
    (This)->lpVtbl -> get_azimuth(This,pAzimuth)

#define IKHViewInfo_put_azimuth(This,azimuth)	\
    (This)->lpVtbl -> put_azimuth(This,azimuth)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKHViewInfo_get_latitude_Proxy( 
    IKHViewInfo * This,
    /* [retval][out] */ double *pLat);


void __RPC_STUB IKHViewInfo_get_latitude_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IKHViewInfo_put_latitude_Proxy( 
    IKHViewInfo * This,
    /* [in] */ double lat);


void __RPC_STUB IKHViewInfo_put_latitude_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKHViewInfo_get_longitude_Proxy( 
    IKHViewInfo * This,
    /* [retval][out] */ double *pLon);


void __RPC_STUB IKHViewInfo_get_longitude_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IKHViewInfo_put_longitude_Proxy( 
    IKHViewInfo * This,
    /* [in] */ double lon);


void __RPC_STUB IKHViewInfo_put_longitude_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKHViewInfo_get_range_Proxy( 
    IKHViewInfo * This,
    /* [retval][out] */ double *pRange);


void __RPC_STUB IKHViewInfo_get_range_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IKHViewInfo_put_range_Proxy( 
    IKHViewInfo * This,
    /* [in] */ double range);


void __RPC_STUB IKHViewInfo_put_range_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKHViewInfo_get_tilt_Proxy( 
    IKHViewInfo * This,
    /* [retval][out] */ double *pTilt);


void __RPC_STUB IKHViewInfo_get_tilt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IKHViewInfo_put_tilt_Proxy( 
    IKHViewInfo * This,
    /* [in] */ double tilt);


void __RPC_STUB IKHViewInfo_put_tilt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKHViewInfo_get_azimuth_Proxy( 
    IKHViewInfo * This,
    /* [retval][out] */ double *pAzimuth);


void __RPC_STUB IKHViewInfo_get_azimuth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IKHViewInfo_put_azimuth_Proxy( 
    IKHViewInfo * This,
    /* [in] */ double azimuth);


void __RPC_STUB IKHViewInfo_put_azimuth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IKHViewInfo_INTERFACE_DEFINED__ */


#ifndef __IKHViewExtents_INTERFACE_DEFINED__
#define __IKHViewExtents_INTERFACE_DEFINED__

/* interface IKHViewExtents */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IKHViewExtents;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D05D6E91-72DA-4654-B8A7-BCBD3B87E3B6")
    IKHViewExtents : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_north( 
            /* [retval][out] */ double *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_south( 
            /* [retval][out] */ double *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_east( 
            /* [retval][out] */ double *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_west( 
            /* [retval][out] */ double *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IKHViewExtentsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IKHViewExtents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IKHViewExtents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IKHViewExtents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IKHViewExtents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IKHViewExtents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IKHViewExtents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IKHViewExtents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_north )( 
            IKHViewExtents * This,
            /* [retval][out] */ double *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_south )( 
            IKHViewExtents * This,
            /* [retval][out] */ double *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_east )( 
            IKHViewExtents * This,
            /* [retval][out] */ double *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_west )( 
            IKHViewExtents * This,
            /* [retval][out] */ double *pVal);
        
        END_INTERFACE
    } IKHViewExtentsVtbl;

    interface IKHViewExtents
    {
        CONST_VTBL struct IKHViewExtentsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IKHViewExtents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IKHViewExtents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IKHViewExtents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IKHViewExtents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IKHViewExtents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IKHViewExtents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IKHViewExtents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IKHViewExtents_get_north(This,pVal)	\
    (This)->lpVtbl -> get_north(This,pVal)

#define IKHViewExtents_get_south(This,pVal)	\
    (This)->lpVtbl -> get_south(This,pVal)

#define IKHViewExtents_get_east(This,pVal)	\
    (This)->lpVtbl -> get_east(This,pVal)

#define IKHViewExtents_get_west(This,pVal)	\
    (This)->lpVtbl -> get_west(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKHViewExtents_get_north_Proxy( 
    IKHViewExtents * This,
    /* [retval][out] */ double *pVal);


void __RPC_STUB IKHViewExtents_get_north_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKHViewExtents_get_south_Proxy( 
    IKHViewExtents * This,
    /* [retval][out] */ double *pVal);


void __RPC_STUB IKHViewExtents_get_south_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKHViewExtents_get_east_Proxy( 
    IKHViewExtents * This,
    /* [retval][out] */ double *pVal);


void __RPC_STUB IKHViewExtents_get_east_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKHViewExtents_get_west_Proxy( 
    IKHViewExtents * This,
    /* [retval][out] */ double *pVal);


void __RPC_STUB IKHViewExtents_get_west_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IKHViewExtents_INTERFACE_DEFINED__ */


#ifndef __IKHFeature_INTERFACE_DEFINED__
#define __IKHFeature_INTERFACE_DEFINED__

/* interface IKHFeature */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IKHFeature;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("07F46615-1857-40CF-9AA9-872C9858E769")
    IKHFeature : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_visibility( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_visibility( 
            /* [in] */ BOOL newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_hasView( 
            /* [retval][out] */ BOOL *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IKHFeatureVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IKHFeature * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IKHFeature * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IKHFeature * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IKHFeature * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IKHFeature * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IKHFeature * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IKHFeature * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_visibility )( 
            IKHFeature * This,
            /* [retval][out] */ BOOL *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_visibility )( 
            IKHFeature * This,
            /* [in] */ BOOL newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_hasView )( 
            IKHFeature * This,
            /* [retval][out] */ BOOL *pVal);
        
        END_INTERFACE
    } IKHFeatureVtbl;

    interface IKHFeature
    {
        CONST_VTBL struct IKHFeatureVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IKHFeature_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IKHFeature_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IKHFeature_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IKHFeature_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IKHFeature_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IKHFeature_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IKHFeature_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IKHFeature_get_visibility(This,pVal)	\
    (This)->lpVtbl -> get_visibility(This,pVal)

#define IKHFeature_put_visibility(This,newVal)	\
    (This)->lpVtbl -> put_visibility(This,newVal)

#define IKHFeature_get_hasView(This,pVal)	\
    (This)->lpVtbl -> get_hasView(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKHFeature_get_visibility_Proxy( 
    IKHFeature * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IKHFeature_get_visibility_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IKHFeature_put_visibility_Proxy( 
    IKHFeature * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB IKHFeature_put_visibility_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKHFeature_get_hasView_Proxy( 
    IKHFeature * This,
    /* [retval][out] */ BOOL *pVal);


void __RPC_STUB IKHFeature_get_hasView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IKHFeature_INTERFACE_DEFINED__ */


#ifndef __IKHInterface_INTERFACE_DEFINED__
#define __IKHInterface_INTERFACE_DEFINED__

/* interface IKHInterface */
/* [unique][helpstring][dual][uuid][object] */ 

typedef /* [public][public][v1_enum] */ 
enum __MIDL_IKHInterface_0001
    {	GE_EC	= 0,
	GE_Pro	= 1,
	GE_Plus	= 2,
	GE_LT	= 3,
	GE_NV	= 4,
	GE_Free	= 5,
	UNKNOWN	= 0xff
    } 	AppType;


EXTERN_C const IID IID_IKHInterface;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("80A43F86-E2CD-4671-A7FA-E5627B519711")
    IKHInterface : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE currentView( 
            /* [in] */ BOOL terrain,
            /* [retval][out] */ IKHViewInfo **pView) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_streamingProgressPercentage( 
            /* [retval][out] */ long *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SaveScreenShot( 
            /* [in] */ BSTR fileName,
            /* [in] */ long quality) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OpenFile( 
            /* [in] */ BSTR fileName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE QuitApplication( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetRenderWindowSize( 
            long width,
            long height) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_autopilotSpeed( 
            /* [retval][out] */ double *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_autopilotSpeed( 
            /* [in] */ double newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_currentViewExtents( 
            /* [retval][out] */ IKHViewExtents **pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setView( 
            /* [in] */ IKHViewInfo *view,
            /* [in] */ BOOL terrain,
            /* [in] */ double speed) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE LoadKml( 
            /* [in] */ BSTR *kmlData) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getFeatureByName( 
            /* [in] */ BSTR name,
            /* [retval][out] */ IKHFeature **pFeature) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setViewParams( 
            /* [in] */ double lat,
            /* [in] */ double lon,
            /* [in] */ double range,
            /* [in] */ double tilt,
            /* [in] */ double azimuth,
            /* [in] */ BOOL terrain,
            /* [in] */ double speed) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setFeatureView( 
            /* [in] */ IKHFeature *feature,
            /* [in] */ double speed) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getPointOnTerrainFromScreenCoords( 
            /* [in] */ double screen_x,
            /* [in] */ double screen_y,
            /* [retval][out] */ SAFEARRAY * *coords) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getCurrentVersion( 
            /* [out] */ int *major,
            /* [out] */ int *minor,
            /* [out] */ int *build,
            /* [out] */ AppType *appType) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE isClientInitialized( 
            /* [out] */ int *isInitialized) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IKHInterfaceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IKHInterface * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IKHInterface * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IKHInterface * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IKHInterface * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IKHInterface * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IKHInterface * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IKHInterface * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *currentView )( 
            IKHInterface * This,
            /* [in] */ BOOL terrain,
            /* [retval][out] */ IKHViewInfo **pView);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_streamingProgressPercentage )( 
            IKHInterface * This,
            /* [retval][out] */ long *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SaveScreenShot )( 
            IKHInterface * This,
            /* [in] */ BSTR fileName,
            /* [in] */ long quality);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *OpenFile )( 
            IKHInterface * This,
            /* [in] */ BSTR fileName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *QuitApplication )( 
            IKHInterface * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetRenderWindowSize )( 
            IKHInterface * This,
            long width,
            long height);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_autopilotSpeed )( 
            IKHInterface * This,
            /* [retval][out] */ double *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_autopilotSpeed )( 
            IKHInterface * This,
            /* [in] */ double newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_currentViewExtents )( 
            IKHInterface * This,
            /* [retval][out] */ IKHViewExtents **pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *setView )( 
            IKHInterface * This,
            /* [in] */ IKHViewInfo *view,
            /* [in] */ BOOL terrain,
            /* [in] */ double speed);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LoadKml )( 
            IKHInterface * This,
            /* [in] */ BSTR *kmlData);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *getFeatureByName )( 
            IKHInterface * This,
            /* [in] */ BSTR name,
            /* [retval][out] */ IKHFeature **pFeature);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *setViewParams )( 
            IKHInterface * This,
            /* [in] */ double lat,
            /* [in] */ double lon,
            /* [in] */ double range,
            /* [in] */ double tilt,
            /* [in] */ double azimuth,
            /* [in] */ BOOL terrain,
            /* [in] */ double speed);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *setFeatureView )( 
            IKHInterface * This,
            /* [in] */ IKHFeature *feature,
            /* [in] */ double speed);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *getPointOnTerrainFromScreenCoords )( 
            IKHInterface * This,
            /* [in] */ double screen_x,
            /* [in] */ double screen_y,
            /* [retval][out] */ SAFEARRAY * *coords);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *getCurrentVersion )( 
            IKHInterface * This,
            /* [out] */ int *major,
            /* [out] */ int *minor,
            /* [out] */ int *build,
            /* [out] */ AppType *appType);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *isClientInitialized )( 
            IKHInterface * This,
            /* [out] */ int *isInitialized);
        
        END_INTERFACE
    } IKHInterfaceVtbl;

    interface IKHInterface
    {
        CONST_VTBL struct IKHInterfaceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IKHInterface_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IKHInterface_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IKHInterface_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IKHInterface_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IKHInterface_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IKHInterface_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IKHInterface_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IKHInterface_currentView(This,terrain,pView)	\
    (This)->lpVtbl -> currentView(This,terrain,pView)

#define IKHInterface_get_streamingProgressPercentage(This,pVal)	\
    (This)->lpVtbl -> get_streamingProgressPercentage(This,pVal)

#define IKHInterface_SaveScreenShot(This,fileName,quality)	\
    (This)->lpVtbl -> SaveScreenShot(This,fileName,quality)

#define IKHInterface_OpenFile(This,fileName)	\
    (This)->lpVtbl -> OpenFile(This,fileName)

#define IKHInterface_QuitApplication(This)	\
    (This)->lpVtbl -> QuitApplication(This)

#define IKHInterface_SetRenderWindowSize(This,width,height)	\
    (This)->lpVtbl -> SetRenderWindowSize(This,width,height)

#define IKHInterface_get_autopilotSpeed(This,pVal)	\
    (This)->lpVtbl -> get_autopilotSpeed(This,pVal)

#define IKHInterface_put_autopilotSpeed(This,newVal)	\
    (This)->lpVtbl -> put_autopilotSpeed(This,newVal)

#define IKHInterface_get_currentViewExtents(This,pVal)	\
    (This)->lpVtbl -> get_currentViewExtents(This,pVal)

#define IKHInterface_setView(This,view,terrain,speed)	\
    (This)->lpVtbl -> setView(This,view,terrain,speed)

#define IKHInterface_LoadKml(This,kmlData)	\
    (This)->lpVtbl -> LoadKml(This,kmlData)

#define IKHInterface_getFeatureByName(This,name,pFeature)	\
    (This)->lpVtbl -> getFeatureByName(This,name,pFeature)

#define IKHInterface_setViewParams(This,lat,lon,range,tilt,azimuth,terrain,speed)	\
    (This)->lpVtbl -> setViewParams(This,lat,lon,range,tilt,azimuth,terrain,speed)

#define IKHInterface_setFeatureView(This,feature,speed)	\
    (This)->lpVtbl -> setFeatureView(This,feature,speed)

#define IKHInterface_getPointOnTerrainFromScreenCoords(This,screen_x,screen_y,coords)	\
    (This)->lpVtbl -> getPointOnTerrainFromScreenCoords(This,screen_x,screen_y,coords)

#define IKHInterface_getCurrentVersion(This,major,minor,build,appType)	\
    (This)->lpVtbl -> getCurrentVersion(This,major,minor,build,appType)

#define IKHInterface_isClientInitialized(This,isInitialized)	\
    (This)->lpVtbl -> isClientInitialized(This,isInitialized)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKHInterface_currentView_Proxy( 
    IKHInterface * This,
    /* [in] */ BOOL terrain,
    /* [retval][out] */ IKHViewInfo **pView);


void __RPC_STUB IKHInterface_currentView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKHInterface_get_streamingProgressPercentage_Proxy( 
    IKHInterface * This,
    /* [retval][out] */ long *pVal);


void __RPC_STUB IKHInterface_get_streamingProgressPercentage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKHInterface_SaveScreenShot_Proxy( 
    IKHInterface * This,
    /* [in] */ BSTR fileName,
    /* [in] */ long quality);


void __RPC_STUB IKHInterface_SaveScreenShot_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKHInterface_OpenFile_Proxy( 
    IKHInterface * This,
    /* [in] */ BSTR fileName);


void __RPC_STUB IKHInterface_OpenFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKHInterface_QuitApplication_Proxy( 
    IKHInterface * This);


void __RPC_STUB IKHInterface_QuitApplication_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKHInterface_SetRenderWindowSize_Proxy( 
    IKHInterface * This,
    long width,
    long height);


void __RPC_STUB IKHInterface_SetRenderWindowSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKHInterface_get_autopilotSpeed_Proxy( 
    IKHInterface * This,
    /* [retval][out] */ double *pVal);


void __RPC_STUB IKHInterface_get_autopilotSpeed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IKHInterface_put_autopilotSpeed_Proxy( 
    IKHInterface * This,
    /* [in] */ double newVal);


void __RPC_STUB IKHInterface_put_autopilotSpeed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IKHInterface_get_currentViewExtents_Proxy( 
    IKHInterface * This,
    /* [retval][out] */ IKHViewExtents **pVal);


void __RPC_STUB IKHInterface_get_currentViewExtents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKHInterface_setView_Proxy( 
    IKHInterface * This,
    /* [in] */ IKHViewInfo *view,
    /* [in] */ BOOL terrain,
    /* [in] */ double speed);


void __RPC_STUB IKHInterface_setView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKHInterface_LoadKml_Proxy( 
    IKHInterface * This,
    /* [in] */ BSTR *kmlData);


void __RPC_STUB IKHInterface_LoadKml_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKHInterface_getFeatureByName_Proxy( 
    IKHInterface * This,
    /* [in] */ BSTR name,
    /* [retval][out] */ IKHFeature **pFeature);


void __RPC_STUB IKHInterface_getFeatureByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKHInterface_setViewParams_Proxy( 
    IKHInterface * This,
    /* [in] */ double lat,
    /* [in] */ double lon,
    /* [in] */ double range,
    /* [in] */ double tilt,
    /* [in] */ double azimuth,
    /* [in] */ BOOL terrain,
    /* [in] */ double speed);


void __RPC_STUB IKHInterface_setViewParams_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKHInterface_setFeatureView_Proxy( 
    IKHInterface * This,
    /* [in] */ IKHFeature *feature,
    /* [in] */ double speed);


void __RPC_STUB IKHInterface_setFeatureView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKHInterface_getPointOnTerrainFromScreenCoords_Proxy( 
    IKHInterface * This,
    /* [in] */ double screen_x,
    /* [in] */ double screen_y,
    /* [retval][out] */ SAFEARRAY * *coords);


void __RPC_STUB IKHInterface_getPointOnTerrainFromScreenCoords_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKHInterface_getCurrentVersion_Proxy( 
    IKHInterface * This,
    /* [out] */ int *major,
    /* [out] */ int *minor,
    /* [out] */ int *build,
    /* [out] */ AppType *appType);


void __RPC_STUB IKHInterface_getCurrentVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IKHInterface_isClientInitialized_Proxy( 
    IKHInterface * This,
    /* [out] */ int *isInitialized);


void __RPC_STUB IKHInterface_isClientInitialized_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IKHInterface_INTERFACE_DEFINED__ */



#ifndef __EARTHLib_LIBRARY_DEFINED__
#define __EARTHLib_LIBRARY_DEFINED__

/* library EARTHLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_EARTHLib;

EXTERN_C const CLSID CLSID_ApplicationGE;

#ifdef __cplusplus

class DECLSPEC_UUID("8097D7E9-DB9E-4AEF-9B28-61D82A1DF784")
ApplicationGE;
#endif

EXTERN_C const CLSID CLSID_TimeGE;

#ifdef __cplusplus

class DECLSPEC_UUID("1AEDB68D-18A7-4CA9-B41B-3CE7E59FAB24")
TimeGE;
#endif

EXTERN_C const CLSID CLSID_TimeIntervalGE;

#ifdef __cplusplus

class DECLSPEC_UUID("42DF0D46-7D49-4AE5-8EF6-9CA6E41EFEC1")
TimeIntervalGE;
#endif

EXTERN_C const CLSID CLSID_CameraInfoGE;

#ifdef __cplusplus

class DECLSPEC_UUID("645EEE5A-BD51-4C05-A6AF-6F2CF8950AAB")
CameraInfoGE;
#endif

EXTERN_C const CLSID CLSID_ViewExtentsGE;

#ifdef __cplusplus

class DECLSPEC_UUID("D93BF052-FC68-4DB6-A4F8-A4DC9BEEB1C0")
ViewExtentsGE;
#endif

EXTERN_C const CLSID CLSID_TourControllerGE;

#ifdef __cplusplus

class DECLSPEC_UUID("77C4C807-E257-43AD-BB3F-7CA88760BD29")
TourControllerGE;
#endif

EXTERN_C const CLSID CLSID_SearchControllerGE;

#ifdef __cplusplus

class DECLSPEC_UUID("A4F65992-5738-475B-9C16-CF102BCDE153")
SearchControllerGE;
#endif

EXTERN_C const CLSID CLSID_AnimationControllerGE;

#ifdef __cplusplus

class DECLSPEC_UUID("1A239250-B650-4B63-B4CF-7FCC4DC07DC6")
AnimationControllerGE;
#endif

EXTERN_C const CLSID CLSID_FeatureGE;

#ifdef __cplusplus

class DECLSPEC_UUID("CBD4FB70-F00B-4963-B249-4B056E6A981A")
FeatureGE;
#endif

EXTERN_C const CLSID CLSID_FeatureCollectionGE;

#ifdef __cplusplus

class DECLSPEC_UUID("9059C329-4661-49B2-9984-8753C45DB7B9")
FeatureCollectionGE;
#endif

EXTERN_C const CLSID CLSID_PointOnTerrainGE;

#ifdef __cplusplus

class DECLSPEC_UUID("1796A329-04C1-4C07-B28E-E4A807935C06")
PointOnTerrainGE;
#endif

EXTERN_C const CLSID CLSID_KHInterface;

#ifdef __cplusplus

class DECLSPEC_UUID("AFD07A5E-3E20-4D77-825C-2F6D1A50BE5B")
KHInterface;
#endif

EXTERN_C const CLSID CLSID_KHViewInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("A2D4475B-C9AA-48E2-A029-1DB829DACF7B")
KHViewInfo;
#endif

EXTERN_C const CLSID CLSID_KHViewExtents;

#ifdef __cplusplus

class DECLSPEC_UUID("63E6BE14-A742-4EEA-8AF3-0EC39F10F850")
KHViewExtents;
#endif

EXTERN_C const CLSID CLSID_KHFeature;

#ifdef __cplusplus

class DECLSPEC_UUID("B153D707-447A-4538-913E-6146B3FDEE02")
KHFeature;
#endif
#endif /* __EARTHLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  LPSAFEARRAY_UserSize(     unsigned long *, unsigned long            , LPSAFEARRAY * ); 
unsigned char * __RPC_USER  LPSAFEARRAY_UserMarshal(  unsigned long *, unsigned char *, LPSAFEARRAY * ); 
unsigned char * __RPC_USER  LPSAFEARRAY_UserUnmarshal(unsigned long *, unsigned char *, LPSAFEARRAY * ); 
void                      __RPC_USER  LPSAFEARRAY_UserFree(     unsigned long *, LPSAFEARRAY * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


