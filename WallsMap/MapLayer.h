//MapLayer.h -- Abstract base class for image and vector map layers
#pragma once

#ifndef _MAPLAYER_H
#define _MAPLAYER_H

#ifndef __GEOREF_H
#include <georef.h>
#endif

#ifndef _FLTPOINT_H
#include "FltPoint.h"
#endif

//#include <trx_str.h>
#include "DIBWrapper.h"
#include "ptnode.h"

class CWallsMapDoc;
class CDlgSymbolsImg;

#define SHP_TOOLTIP_SIZ 40
#define PROP_COLUMNS 3
#define PROP_COL_MARGIN 12

#define NTL_FLG_ALLSAVED (NTL_FLG_SAVEHISTORY|NTL_FLG_AVERAGE|NTL_FLG_FEETUNITS|NTL_FLG_RESTOREVIEW|NTL_FLG_ELEVUNITSMETERS|NTL_FLG_DATUMTOGGLED|NTL_FLG_SAVELAYERS)

enum {NTL_FLG_HIDDEN=0,
	  NTL_FLG_VISIBLE=1,
	  NTL_FLG_MARKERS=2,
	  NTL_FLG_LABELS=4,
	  NTL_FLG_AVERAGE=8,
	  NTL_FLG_EDITABLE=16,
	  NTL_FLG_SHOWMARKERS=32,
	  NTL_FLG_SHOWLABELS=64,
	  NTL_FLG_NONSELECTABLE=128,
	  NTL_FLG_FEETUNITS=256,
	  NTL_FLG_SEARCHEXCLUDED=512,
	  NTL_FLG_RESTOREVIEW=1024,
	  NTL_FLG_ELEVUNITSMETERS=2048,
	  NTL_FLG_DATUMTOGGLED=4096,
	  NTL_FLG_SAVEHISTORY=8192,
	  NTL_FLG_TRISTATE=16384,
	  NTL_FLG_SAVELAYERS=32768,
	  NTL_FLG_TESTMEMOS=65536,
	  NTL_FLG_LAYERSET=(1<<31),
	  NTL_FLG_SILENT=(1<<30)
};

#define NTL_FLG_OPENONLY (NTL_FLG_LAYERSET|NTL_FLG_SILENT)
#define V_FLG (NTL_FLG_VISIBLE|NTL_FLG_TRISTATE)
#define V_F(flg) (flg&V_FLG)
#define V_PF(ptl) (ptl->m_uFlags&V_FLG)

#define NADZON_OK(nad,zon) ((nad==1 || nad==2) && abs(zon)<=60)

//Saved settings in properties dialog --
class CPropStatus {
public:
	CPropStatus() : nPos(0),nWinWidth(0) {nColWidth[0]=0;}
	int nTab;
	int nPos;
	int nColWidth[PROP_COLUMNS];
	int nWinWidth,nWinHeight;
};

__inline double MetricDistance(CFltPoint &fp1,CFltPoint &fp2)
{
	return MetricDistance(fp1.y,fp1.x,fp2.y,fp2.x);
}

__inline int ContainsRect(CRect bigRect,const CRect &rect)
{
	return bigRect.IntersectRect(bigRect,rect)?(1+(bigRect==rect)):0;
}

__inline bool extentOverlap(const CFltRect &fr1,const CFltRect &fr2)
{
	if(fr1.l>=fr2.r || fr2.l>=fr1.r) return false;
	int iSign=(fr1.t<=fr1.b)?1:-1;
	return iSign*(fr1.b-fr2.t)>0.0 && iSign*(fr2.b-fr1.t)>0.0;
}

__inline bool	trimExtent(CFltRect &ext,const CFltRect &extent)
{
	// Trim parts of ext not contained in extent
	// returns:	false if result is empty, true otherwise

	if(ext.l<extent.l) ext.l=extent.l;
	if(ext.r>extent.r) ext.r=extent.r;
	if(ext.r-ext.l<=0.0) return false;
	int iSign=(extent.t<=extent.b)?1:-1;
	if(iSign*(ext.t-extent.t)<0.0) ext.t=extent.t;
	if(iSign*(ext.b-extent.b)>0.0) ext.b=extent.b;
	return iSign*(ext.b-ext.t)>0.0;
}

struct CScaleRange {
	CScaleRange() :
		bLayerNoShow(0),bLabelNoShow(0),
		uLayerZoomIn(0),uLayerZoomOut(0),
		uLabelZoomIn(0),uLabelZoomOut(0) {}
	void Clear()
	{
		bLayerNoShow=bLabelNoShow=0;
		uLayerZoomIn=uLayerZoomOut=uLabelZoomIn=uLabelZoomOut=0;
	}
	short bLayerNoShow;
	short bLabelNoShow;
	UINT  uLayerZoomIn;
	UINT  uLayerZoomOut;
	UINT  uLabelZoomIn;
	UINT  uLabelZoomOut;
};

class CShpLayer;

struct SHPREC {
	SHPREC() {}
	SHPREC(CShpLayer *pS,DWORD r) : pShp(pS),rec(r) {}
	CShpLayer *pShp;
	DWORD rec;
};

typedef std::vector<SHPREC> VEC_SHPREC;
typedef VEC_SHPREC::iterator it_shprec;

enum {TYP_FOLDER,TYP_SHP,TYP_NTI,TYP_NTL,TYP_GDAL};
class CTreeLayer;
typedef CTreeLayer *PTL;

class CTreeLayer
{
public:
	CTreeLayer():m_uFlags(0),m_uLevel(0),m_nImage(2),m_hItem(0),parent(0),child(0),prevsib(0),nextsib(0) {}
	CTreeLayer(LPCSTR title):m_uFlags(0),m_uLevel(0),m_nImage(0),m_hItem(0),parent(0),child(0),prevsib(0),nextsib(0),m_csTitle(title) {}
	virtual ~CTreeLayer();
	CString m_csTitle;
	UINT m_uFlags;
	UINT m_uLevel;
	int m_nImage;
	PTL parent,child,prevsib,nextsib;
	HTREEITEM m_hItem;
	bool IsFolder() const {return m_nImage<=1;}
	bool IsVisible() const {return (m_uFlags&NTL_FLG_VISIBLE)!=0;}
	bool IsDisplayed() const {return V_F(m_uFlags)==NTL_FLG_VISIBLE;}
	bool IsTristate() const {return (m_uFlags&NTL_FLG_TRISTATE)!=0;}
	bool IsImageType() {return m_nImage>1 && LayerType()!=TYP_SHP;}
	void SetTitle(LPCSTR lpszTitle) {m_csTitle=lpszTitle;}
	LPCSTR Title() const {return (LPCSTR)m_csTitle;}

	LPCSTR LayerPath(CString &path);

	PTL FirstChildPML()
	{
		//Return first PML beneath folder ptl, or NULL if none
		for(PTL ptl=child;ptl;ptl=ptl->nextsib) {
			if(!ptl->IsFolder()) return ptl;
			PTL p=ptl->FirstChildPML();
			if(p) return p;
		}
		return false;
	}

	bool HasVisibleChildren()
	{
		for(PTL ptl=child;ptl;ptl=ptl->nextsib) {
			if(ptl->IsVisible()) return true;
		}
		return false;
	}

	virtual int LayerType() const;
	virtual LPCSTR GetInfo();
	virtual bool SetVisible(bool bVisible);
};

typedef std::vector<PTL> VEC_TREELAYER;
typedef VEC_TREELAYER::reverse_iterator rit_Layer;
typedef VEC_TREELAYER::iterator it_Layer;

class CMapLayer;
typedef CMapLayer *PML;
//========================================================================

class CMapLayer : public CTreeLayer
{
friend class CLayerSet;
public:

	virtual ~CMapLayer();
	CMapLayer() : m_pathName(NULL)
		,m_datumName(NULL)
		,m_unitsName(NULL)
		,m_projName(NULL)
		,m_projectionRef(NULL)
		,m_bWGS84(false)
		,m_bOpen(false)
		,m_bTrans(false)
		,m_bProjected(false)
		,m_bNullExtent(false)
		,m_bSearchable(false)
		,m_iZone(-99)
		,m_iNad(0)
		,m_pDoc(NULL)
	{
		ClearTransforms();
	}

	virtual int CopyToDIB(CDIBWrapper *pDestDIB,const CFltRect &geoext,double fScale)=0;
	virtual BOOL Open(LPCTSTR lpszPathName,UINT uFlags)=0;
	virtual void AppendInfo(CString &s) const = 0;
	virtual LPCSTR GetInfo();
	void FreeDupedStrings();
	bool GetPrjProjection(LPCSTR pPath);
	bool GetProjection(LPCSTR pRef);
	bool AppendProjectionRefInfo(CString &s) const;
	void InitDlgTitle(CDialog *pDlg) const;
	bool IsAppendable() const;
	LPCSTR ProjectionRef() const {return m_projectionRef;}

	void DrawOutline(CDC *pDC,CFltRect &rectGeo,double fScale,CBrush *pBrush);
	LPCSTR GetPrefixedTitle(CString &cs) const;
	LPCSTR PathName() const {return m_pathName;}
	int Zone() {return m_iZone;}
	int Nad() {return m_iNad;}
	LPSTR GetCoordSystemStr() const;
	//LPSTR CMapLayer::GetKnownCoordSystemStr() const;
	static LPCSTR GetShortCoordSystemStr(int iNad,int iZone);
	LPCSTR DatumName() const {return m_datumName;}
	LPCSTR UnitsName() const {return m_unitsName;}
	LPCSTR FileName() const {return trx_Stpnam(m_pathName);}
	LPCSTR SizeStr() const {return (LPCSTR)m_csSizeStr;}
	bool IsSearchExcluded() const {return (m_uFlags&NTL_FLG_SEARCHEXCLUDED)!=0;}
	void SetSearchExcluded(bool bSet)
	{
		if(bSet) m_uFlags|=NTL_FLG_SEARCHEXCLUDED;
		else m_uFlags&=~NTL_FLG_SEARCHEXCLUDED;
	}
	int Visibility(DWORD rec=0);

	bool IsSyncCompatible(const CLayerSet *pLSet);
	bool IsNullExtent() {return m_bNullExtent;}

	virtual bool SetVisible(bool bVisible);

	bool IsOpen() {return m_bOpen;}

	CFltRect & Extent() {return m_extent;}

	bool ExtentOverlap(const CFltRect &ext)
	{
		return extentOverlap(m_extent,ext);
	}

	void ImgPtToGeoPt(CFltPoint &imgPos) const
	{
		if(m_bTrans) {
			double x=imgPos.x;
			imgPos.x=m_fTransImg[0]+m_fTransImg[1]*x+m_fTransImg[2]*imgPos.y;
			imgPos.y=m_fTransImg[3]+m_fTransImg[4]*x+m_fTransImg[5]*imgPos.y;
		}
	}

	void GeoPtToImgPt(CFltPoint &geoPos) const
	{
		if(m_bTrans) {
			double x=geoPos.x;
			geoPos.x=m_fTransGeo[0]+m_fTransGeo[1]*x+m_fTransGeo[2]*geoPos.y;
			geoPos.y=m_fTransGeo[3]+m_fTransGeo[4]*x+m_fTransGeo[5]*geoPos.y;
		}
	}

	void ImgPtToGeoPt(const CFltPoint &imgPos,CFltPoint &geoPos) const
	{
		if(m_bTrans) {
			geoPos.x=m_fTransImg[0]+m_fTransImg[1]*imgPos.x+m_fTransImg[2]*imgPos.y;
			geoPos.y=m_fTransImg[3]+m_fTransImg[4]*imgPos.x+m_fTransImg[5]*imgPos.y;
		}
		else geoPos=imgPos;
	}

	void GeoPtToImgPt(const CFltPoint &geoPos,CFltPoint &imgPos) const
	{
		if(m_bTrans) {
			imgPos.x=m_fTransGeo[0]+m_fTransGeo[1]*geoPos.x+m_fTransGeo[2]*geoPos.y;
			imgPos.y=m_fTransGeo[3]+m_fTransGeo[4]*geoPos.x+m_fTransGeo[5]*geoPos.y;
		}
		else imgPos=geoPos;
	}

	bool TrimExtent(CFltRect &ext) const
	{
		return trimExtent(ext,m_extent);
	}

	bool LayerScaleRangeOK(UINT scaleDenom)
	{
		return !scaleRange.bLayerNoShow ||
			(scaleRange.uLayerZoomIn<=scaleDenom && (!scaleRange.uLayerZoomOut || scaleRange.uLayerZoomOut>scaleDenom));
	}

	bool LabelScaleRangeOK(UINT scaleDenom)
	{
		return !scaleRange.bLabelNoShow ||
			(scaleRange.uLabelZoomIn<=scaleDenom && (!scaleRange.uLabelZoomOut || scaleRange.uLabelZoomOut>scaleDenom));
	}

	bool IsEditable() const {return (m_uFlags&NTL_FLG_EDITABLE)!=0;}
	bool IsProjected() {return m_bProjected;}
	bool IsTransformed() {return m_bTrans;}
	bool IsSearchable() {return m_bSearchable;}
	double *GetTransImg() {return m_fTransImg;}
	double MetersPerUnit() {return m_fMetersPerUnit;}
	bool IsGeoRefSupported() const {return NADZON_OK(m_iNad,m_iZone);} 

	CScaleRange scaleRange;
	CWallsMapDoc *m_pDoc;

	bool m_bSearchable; //is a point shapefile

protected:
	BOOL IsLatLongExtent();
	void ComputeInverseTransform();
	void ClearTransforms()
	{
		m_fTransImg[1]=m_fTransImg[5]=1.0;
		m_fTransImg[0]=m_fTransImg[2]=m_fTransImg[3]=m_fTransImg[4]=0.0;
		m_fTransGeo[1]=m_fTransGeo[5]=1.0;
		m_fTransGeo[0]=m_fTransGeo[2]=m_fTransGeo[3]=m_fTransGeo[4]=0.0;
	}
	CString m_csSizeStr;
	CString m_csInfo;
	LPCSTR m_projName;
	LPCSTR m_pathName;
	LPCSTR m_datumName;
	LPCSTR m_unitsName;
	LPCSTR m_projectionRef; //NULL if not georeferenced
	double m_fMetersPerUnit;
	double m_fTransImg[6];
	double m_fTransGeo[6];
	CFltRect m_extent;
	bool m_bOpen;
	bool m_bTrans;
	bool m_bProjected;
	bool m_bNullExtent;
	bool m_bWGS84;
	INT8 m_iZone; //1..60, -1..-60 if UTM, 0 if Lat/Long, -99 if unspecified projection, 99 if unsupported projection
	int m_iNad; //0 if unspecified, 1 if NAD83, 2 if NAD27, 3 if unsupported
};

class CImageLayer : public CMapLayer 
{
public:

	enum {DFLT_TOLERANCE=4}; // 0-based index into [0 2 4 8 16 32 64]

	CImageLayer() :
	     m_lpBitmapInfo(NULL)
		,m_bAlpha(0xff)
		,m_bUseTransColor(false)
		,m_hdcMask(NULL)
		,m_iLastTolerance(DFLT_TOLERANCE)
		,m_crTransColor(0)
		,m_crTransColorMask(0) {}


	virtual ~CImageLayer();
	static const UINT32 BlkWidth=256;
	static const UINT32 MaxLevels=16;

	bool ComputeLevelLimit(BYTE &bLevelLimit,UINT32 uWidth,UINT32 uHeight);

	UINT32 NumColorEntries() {return m_uColors;}
	UINT32 NumBands() {return m_uBands;}
	UINT32 Width() {return m_uWidth;}
	UINT32 Height() {return m_uHeight;}
	UINT32 BitsPerSample() {return m_uBitsPerSample;}
	UINT32 Bands() {return m_uBands;}
	LPBITMAPINFO BitmapInfo() {return m_lpBitmapInfo;}
	void GeoRectToImgRect(const CFltRect &geoExt,CRect &imgRect) const;

	void DeleteDIB() {m_dib.DeleteObject();}

	virtual int CopyAllToDC(CDC *pCDC,const CRect &crDst,HBRUSH hbr=NULL) = 0;
	virtual BOOL OpenHandle(LPCTSTR lpszPathName,UINT uFlags=0) = 0;
	virtual void CloseHandle() = 0;
	virtual void CopyBlock(LPBYTE pDstOrg,LPBYTE pSrcOrg,int srcLenX,int srcLenY,int nSrcRowWidth)=0;

	void GetNameAndSize(CString &s,LPCSTR path,bool bDate=false);
	
	//Variables for AlphaBlend() and transparent color operations
	static CDlgSymbolsImg *m_pDlgSymbolsImg;
	BYTE m_bAlpha,m_;
	bool m_bUseTransColor;
	int m_iLastTolerance;
	COLORREF m_crTransColor,m_crTransColorMask;
	CBitmap m_mask;
	HDC m_hdcMask;
	HBITMAP m_hbmMaskOrg;

	bool IsTransparent() const
	{
		return (m_bUseTransColor || m_bAlpha<0xff);
	}

	UINT NumOvrBands() const
	{
		return m_uOvrBands;
	}

	bool IsUsingBGRE() const
	{
		return (m_crTransColor>>24)!=0;
	}

	static int GetTolerance(const COLORREF &clr)
	{
		// returns 0 <= tol <= 6
		BYTE bError=(clr>>24);
		if(bError<2) return 0;
		int tol=0;
		for(;bError>>=1;tol++);
		return tol;
	}

	static BYTE GetToleranceError(int tolerance)
	{
		return tolerance?(1<<tolerance):0;
	}

	void ApplyBGRE();
	void DeleteMask();

protected:

	void AppendGeoRefInfo(CString &s) const;
	void InitSizeStr();
	//virtual BOOL GetDIBits()=0;
	virtual void AppendInfo(CString &s) const = 0;
	bool CopyData(CDIBWrapper *pDestDIB,int destOffX,int destOffY,int destWidth,int destHeight,
		int srcOffX,int srcOffY,int srcWidth,int srcHeight);

	void CreateMask(HDC hdc);

	void ApplyMask(HDC destDC,int destOffX,int destOffY,int destWidth,int destHeight,
		int srcOffX,int srcOffY,int srcWidth,int srcHeight);
	
	void ComputeCellRatios()
	{
		double fSizeY=fabs(m_fTransImg[5]);
		double fSizeX=m_fTransImg[1];
		if(fSizeX<=fSizeY) {
			m_fCellSize=fSizeX;
			m_fCellRatioX=1.0;
			m_fCellRatioY=fSizeY/fSizeX; //cell heights cover more screen pixels than widths
		}
		else {
			m_fCellSize=fSizeY;
			m_fCellRatioY=1.0;
			m_fCellRatioX=fSizeX/fSizeY; //cell widths cover more screen pixels than heights
		}
	}

	void ComputeExtent()
	{
		ImgPtToGeoPt(CFltPoint(0,0),m_extent.tl);
		ImgPtToGeoPt(CFltPoint(m_uWidth,m_uHeight),m_extent.br);
	}

	UINT32 m_uWidth,m_uHeight; //Extent of data in native pixel units (100% zoom)
	UINT32 m_uBands,m_uColors,m_uOvrBands,m_uBitsPerSample;

	LPBITMAPINFO m_lpBitmapInfo;
	CDIBWrapper m_dib;
	int m_dib_lvl;
	CRect m_dib_rect;
	bool m_bPalette;
	BYTE m_bLevelLimit; //number of overviews, or last NTI lvl index
	double m_fCellRatioX,m_fCellRatioY;

public:
	double m_fCellSize;
};

static int __inline _rnd(double x)
{
	return (int)((x>=0)?(x+0.5):(x-0.5));
}

double rint(double x); //defined in layerset.cpp

#define SHP_EXT CLayerSet::m_ext[TYP_SHP]
#define NTL_EXT CLayerSet::m_ext[TYP_NTL]
#define NTI_EXT CLayerSet::m_ext[TYP_NTI]

class CNtiLayer;

class CLayerSet
{
public:

	static const LPCSTR m_ext[TYP_GDAL];

	CLayerSet() : m_datumName(NULL)
		, m_unitsName(NULL)
		, m_iZone(-99)
		, m_iNad(0)
		, m_bProjected(false)
		, m_bTrans(false)
		, m_bShpPtLayers(false)
		, m_bShpLayers(false)
		, m_bSelectedSetFlagged(true)
		, m_bChanged(0)
		, m_pLayerMaxImagePixelsPerGeoUnit(NULL)
		, m_fMaxImagePixelsPerGeoUnit(1.0)
		, m_pImageList(NULL)
	{
	}

	~CLayerSet() {
		delete m_pImageList;
		free(m_unitsName);
		free(m_datumName);
		for(it_Layer p=m_layers.begin();p!=m_layers.end();++p) {
			delete *p;
		}
		VEC_TREELAYER().swap(vAppendLayers); //why not
	}

	// Called from CPageLayers() after rearranging tree --
	void ReplaceLayerSet(VEC_TREELAYER &vec) {m_layers.swap(vec);}

	PML TopMapLayer() {return InitTreePML();}

	void PushBackPTL(PTL ptl) {m_layers.push_back(ptl);}

	void InsertPTL(rit_Layer rit,PTL ptl) {m_layers.insert(rit.base(),ptl);}

	void CreateImageList();

	static int GetLayerType(LPCSTR pathname);

	double MaxImagePixelsPerGeoUnit() {return m_fMaxImagePixelsPerGeoUnit;}

	CImageLayer *GetTopImageLayer(const CFltPoint &ptGeo);
	CImageLayer *GetTopNteLayer(const CFltPoint &ptGeo);

	double ImagePixelsPerGeoUnit(const CFltPoint &ptGeo);

	UINT MaxPixelWidth(const CFltRect &geoRect);

	BOOL OpenLayer(LPCSTR lpszPathName,LPCSTR lpszTitle,UINT uFlags,int pos=-1,int posSib=0);
	void PrepareLaunchPt(CFltPoint &fpt);

	int CopyToDIB(CDIBWrapper *pDestDIB,const CFltRect &rectGeo,double fScale);
	BOOL CopyToImageDIB(CDIBWrapper *pDestDIB,const CFltRect &rectGeo);
	bool GetScaledExtent(CFltRect &fRect,double fScale);
	int GetAppendableLayers(CMapLayer *pShp);
	HMENU AppendCopyMenu(CMapLayer *pLayer,CMenu *pPopup,LPCSTR pItemName);

	BOOL IsLayerCompatible(CMapLayer *pLayer,BOOL bSilent=FALSE);
	bool HasSearchableLayers();
	BOOL IsSetSearchable(BOOL bActive);
	void ClearSelChangedFlags();
	void RefreshTables();

	bool IsGeoWgs84() {return m_iZone==0 && m_iNad==1;}
	int NumLayers() const {return (int)m_layers.size();}

	PML MapLayerPtr(int iPos) {return (PML)m_layers.rbegin()[iPos];}
	PTL LayerPtr(int iPos) {return m_layers.rbegin()[iPos];}
	PTL SelectedLayerPtr() {return m_layers.rbegin()[m_PropStatus.nPos];}
	const CMapLayer &MapLayer(int iPos) const {return *(PML)m_layers.rbegin()[iPos];}
	const CTreeLayer &Layer(int iPos) const {return *m_layers.rbegin()[iPos];}
	const CTreeLayer &SelectedLayer() const {return *m_layers.rbegin()[m_PropStatus.nPos];}
	const int SelectedLayerPos() const {return m_PropStatus.nPos;}
	void SetSelectedLayerPos(int pos)
	{
		m_PropStatus.nPos=pos;
	}
	rit_Layer SelectedLayerRit() {return m_layers.rbegin()+m_PropStatus.nPos;}
	it_Layer SelectedLayerIt() {return m_layers.end()-m_PropStatus.nPos-1;}
	it_Layer BeginLayerIt() {return m_layers.begin();}
	it_Layer EndLayerIt() {return m_layers.end();}
	rit_Layer BeginLayerRit() {return m_layers.rbegin();}
	rit_Layer EndLayerRit() {return m_layers.rend();}
	void DrawOutlines(CDC *pDC,CFltRect &rectGeo,double fScale);
	BOOL InsertLayer(CMapLayer *pLayer,UINT uFlags,int pos,int posSib);
	int IsLayer(LPCSTR lpszPathName);
	void ComputeExtent();
	void UpdateExtent() {GetExtent(&m_extent,FALSE);}
	void SaveShapefiles(); //Return number not successfully saved
	bool FindByLabel(const CString &text,WORD wFlags);
	int SelectEditedShapes(UINT uFlags,BOOL bAddToSel,CFltRect *pRectView);

	void InitLayerIndices();
	void UnflagSelectedRecs();
	void FlagSelectedRecs();
#ifdef _DEBUG
	//static void CompactSrchVecShprec(int iStart);
#endif
	void StoreSelectedRecs();
	//The following 4 fcns operate on the selected layer --

	void DeleteLayer();
	void ReplaceLayer(CMapLayer *pLayer);
	void AddNtiLayer(CString &path);

	bool IsGeoRefSupported() const {return NADZON_OK(m_iNad,m_iZone);} 
	bool HasEditedShapes();

	bool ExtentOverlap(const CFltRect &ext)
	{
		//Note: the layerset's extent can have "holes" --
		for(PML pml=InitPML();pml;pml=NextPML())
			if(pml->ExtentOverlap(ext)) return true;
		return false;
	}

	int FirstVisibleIndex()
	{
		for(PML pml=InitPML();pml;pml=NextPML()) {
			if(pml->IsVisible()) return m_it-m_layers.begin();
		}
		return -1;
	}

	int LayerPos(CMapLayer *pLayer)
	{
		for(PML pml=InitPML();pml;pml=NextPML()) {
			if(pml==pLayer) return m_layers.end()-m_it;
		}
		return 0;
	}

	bool TrimExtent(CFltRect &ext)
	{
		return trimExtent(ext,m_extent);
	}

	const CFltRect & Extent() {return m_extent;}

	void GetExtent(CFltRect *pfRect,BOOL bVisible);

	CFltPoint ExtentSize() {return m_extent.Size();}

	void GetOtherDatumName(CString &name);

	void GetDatumName(CString &name)
	{
		if(m_datumName && strlen(m_datumName)<8) name.SetString(m_datumName);
	}

	int GetUTMOtherDatum(CFltPoint &fpt);
	void GetDEGOtherDatum(CFltPoint &fpt);

	int GetUTMCoordinates(CFltPoint &fpt)
	{
		if(IsProjected())
			return m_iZone;
		ASSERT(!m_iZone);
		int iZone=0;
		geo_LatLon2UTM(fpt,&iZone,(m_iNad==2)?geo_NAD27_datum:geo_WGS84_datum);
		return iZone;
	}

	void GetDEGCoordinates(CFltPoint &fpt)
	{
		if(IsProjected()) {
			ASSERT(m_iZone);
			geo_UTM2LatLon(fpt,m_iZone,(m_iNad==2)?geo_NAD27_datum:geo_WGS84_datum);
		}
	}

	bool IsValidGeoPt(const CFltPoint &fpt)
	{
		CFltPoint pt(fpt);
		if(IsProjected()) {
			ASSERT(m_iZone);
			GetDEGCoordinates(pt);
		}
		return fabs(pt.y)<90.0 && fabs(pt.x)<180.0;
	}

	bool IsProjected() const {return m_bProjected;}
	bool IsTransformed() const {return m_bTrans;}
	static UINT GetScaleDenom(const CFltRect &rectGeo,double fScale,bool bProj);

	bool IsSyncCompatible(const CLayerSet &lSet)
	{
#ifdef _DEBUG
		if(&lSet==this) {
			ASSERT(FALSE);
		}
#endif
		if(!m_bTrans) return !lSet.m_bTrans;
		return (lSet.m_bTrans && IsGeoRefSupported() && lSet.IsGeoRefSupported());
	}

	BOOL IsChanged() const {return m_bChanged;}
	void SetChanged(BOOL bChanged=2) {
		if(m_bChanged<bChanged) m_bChanged=bChanged;
	}
	void SetUnChanged() {m_bChanged=0;}

	bool HasNteLayers() {return m_nteLayers.size()!=0;}
	bool ShpLayersVisible(UINT flag,UINT minAnd=0);
	bool ImageLayersPresent();
	int NumLayersVisible();
	int NumMapLayers();
	int Zone() {return m_iZone;}
	int Nad() {return m_iNad;}
	void InitPtNode(CDIBWrapper *pDestDIB,const CFltRect &rectGeo);
	bool InitVecShpopt(bool bAdv);

	PML InitTreePML() {m_rit=BeginLayerRit(); return NextTreePML();}
	PML NextTreePML()
	{
		for(;m_rit!=m_layers.rend();m_rit++) {
			if(!(*m_rit)->IsFolder()) return (PML)*m_rit++;
		}
		return NULL;
	}
	PML InitPML()
	{
		m_it=BeginLayerIt();
		return NextPML();
	}
	PML NextPML()
	{
		for(;m_it!=m_layers.end();m_it++) {
			if(!(*m_it)->IsFolder()) return (PML)*m_it++;
		}
		return NULL;
	}

	//==================================================

	LPSTR m_datumName,m_unitsName;
	//m_iNad= 0:unspecified, 1:NAD83, 2:NAD27, 3:specified but not NAD27/83
	INT8 m_iZone,m_iNad;
	bool m_bTrans,m_bProjected,m_bShpPtLayers,m_bShpLayers;
	bool m_bSelectedSetFlagged; //for checking logic
	static bool m_bReadWriteConflict,m_bPtNodeInitialized;
	static int m_bOpenConflict;
	static UINT m_uScaleDenom;
	static rit_Layer m_rit;
	static it_Layer m_it;

	static VEC_TREELAYER vAppendLayers;
	static VEC_SHPREC vec_shprec_srch;
	static void Empty_shprec_srch() {
		VEC_SHPREC().swap(vec_shprec_srch);
	}
	static bool m_bAddingNtiLayer;
	static bool m_bExporting;
	static bool bAddingNtiLayer;
	static double m_fExportScaleRatio;
	static CLayerSet *m_pLsetImgSource;

	CMapLayer *m_pLayerMaxImagePixelsPerGeoUnit;
	double m_fMaxImagePixelsPerGeoUnit;
	CPropStatus m_PropStatus;
	CImageList *m_pImageList;
	CWallsMapDoc *m_pDoc;
	BOOL m_bChanged;
private:
	void InitDatumNames(CMapLayer *pLayer);

	CFltRect m_extent;
	VEC_TREELAYER m_layers;
	VEC_TREELAYER m_nteLayers;
};

#endif
