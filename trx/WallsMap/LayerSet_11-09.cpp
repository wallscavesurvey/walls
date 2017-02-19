//CLayerSet fcns --

#include "stdafx.h"
#include "resource.h"
#include "MapLayer.h"
#include "WallsMap.h"
#include "WallsMapDoc.h"
#include "DIBWrapper.h"
#include "NtiLayer.h"
#include "GdalLayer.h"
#include "ShowShapeDlg.h"
#include "EditAccent.h"
#include "ImageViewDlg.h"
#include "TabCombo.h"
#include "mainfrm.h"
#include "WallsMapView.h"
#include "utility.h"
#include "DBGridDlg.h"
#include <georef.h>

//Utility that should be in standard math library, but oddly is not --

/*
double rint( double x)
// Copyright (C) 2001 Tor M. Aamodt, University of Toronto
// Permisssion to use for all purposes commercial and otherwise granted.
// THIS MATERIAL IS PROVIDED "AS IS" WITHOUT WARRANTY, OR ANY CONDITION OR
// OTHER TERM OF ANY KIND INCLUDING, WITHOUT LIMITATION, ANY WARRANTY
// OF MERCHANTABILITY, SATISFACTORY QUALITY, OR FITNESS FOR A PARTICULAR
// PURPOSE.
{
    if( x > 0 ) {
        __int64 xint = (__int64) (x+0.5);
        if( xint % 2 ) {
            // then we might have an even number...
            double diff = x - (double)xint;
            if( diff == -0.5 )
                return double(xint-1);
        }
        return double(xint);
    } else {
        __int64 xint = (__int64) (x-0.5);
        if( xint % 2 ) {
            // then we might have an even number...
            double diff = x - (double)xint;
            if( diff == 0.5 )
                return double(xint+1);
        }
        return double(xint);
    }
}
*/
#if 0
static int spcx,spcy;
static int iLabelSpace,mapWidth,mapDiv;
static UINT uLabelCnt,uLabelInc;
static WORD *pLabelMap;
static CSize mapSize;

static BOOL CheckRanges(int &x1,int &y1,int &x2,int &y2)
{
    if(x1<0) x1=0;
    if(x2>=mapSize.cx) x2=mapSize.cx-1;
    if(y1<0) y1=0;
    if(y2>=mapSize.cy) y2=mapSize.cy-1;
    return x1<x2 && y1<y2;
}

static apfcn_i TestMap(int x1,int y1,int x2,int y2)
{ 
    WORD mx1,mx2;
	WORD  *pw;
    WORD  *hpw;
    
    if(!CheckRanges(x1,y1,x2,y2)) return TRUE;
    x1/=mapDiv; x2/=mapDiv;
    y1/=mapDiv; y2/=mapDiv;
    mx1=(0xFFFF<<(x1%16));
    x1/=16;
    mx2=(0xFFFF>>(16-(x2%16)));
    x2/=16;
    hpw=pLabelMap+((long)y1*mapWidth+x1);
    for(int i=y1;i<y2;i++) {
      pw=hpw;
      if((*pw&mx1) || (pw[x2-x1]&mx2)) return TRUE;
      for(int j=x2-x1-1;j>0;j--) {
        if(*++pw) return TRUE;
	    ASSERT(pw<hlimit);
      }
      hpw+=mapWidth;
    }
    return FALSE;
}

static apfcn_v SetMap(int x1,int y1,int x2,int y2)
{ 
    WORD mx1,mx2;
    WORD *pw;
    WORD *hpw;
    
    if(!CheckRanges(x1,y1,x2,y2)) return;
    x1/=mapDiv; x2/=mapDiv;
    y1/=mapDiv; y2/=mapDiv;
    mx1=(0xFFFF<<(x1%16));
    x1/=16;
    mx2=(0xFFFF>>(16-(x2%16)));
    x2/=16;
    hpw=pLabelMap+((long)y1*mapWidth+x1);
    for(int i=y1;i<y2;i++) {
      pw=hpw;
      *pw |= mx1;
      pw[x2-x1] |= mx2;
      for(int j=x2-x1-1;j>0;j--) *++pw |= 0xFFFF;
	  ASSERT(pw<hlimit);
      hpw+=mapWidth;
    }
}

static apfcn_v InitLabelMap()
{
	ASSERT(!pLabelMap);
	//Allocate a map of size less than 256K bytes --
	mapDiv=1;
	do {
		 mapDiv++;
		 mapWidth=(mapSize.cx/mapDiv+15)/16;
	}
	while((long)(mapSize.cy/mapDiv)*mapWidth>=1024*128L);
	       
	if(NULL==(pLabelMap=(WORD *)calloc(mapWidth*((long)mapSize.cy/mapDiv),sizeof(WORD)))) {
		 iLabelSpace=0;
		 return;
	}
    #ifdef _DEBUG
    hlimit=pLabelMap+mapWidth*((long)mapSize.cy/mapDiv);
    #endif
    
    //Necessary, since printer may have changed --
	if(pMF->bPrinter) {
	  CFont cf;
	  CFont *pcfOld=pMF->pfLabelFont->ScalePrinterFont(pvDC,&cf);
	  pvDC->SelectObject(pcfOld);
	}
	spcx=pMF->pfLabelFont->AveCharWidth;
	spcy=pMF->pfLabelFont->LineHeight;
	spcx=spcx*(iLabelSpace-1)+spcx/2;
	spcy=(spcy/2)*(iLabelSpace-1);
}

static apfcn_i LabelMap(int x,int y,char *p,int len)
{
    //If !pLabelMap, allocate zeroed map. Set iLabelSpace=0 and return TRUE upon failure.
    //With map allocated, return FALSE if map region not clear, else turn region
    //bits on and return TRUE.
    
    CSize size;
    
    ASSERT(pLabelMap);
    
    if(len<0) {
      //Calculate size of box around multiline text--
      int xlen=0,ylen=0;
      char *p0=p,*p1;
      while(p0<p-len) {
        if(!(p1=strchr(p0,'\n'))) p1=p-len;
        size=pvDC->GetTextExtent(p0,p1-p0);
        ylen+=size.cy; //Same as pMF->pfNoteFont->LineHeight?
        if(size.cx>xlen) xlen=size.cx;
        p0=p1+1;
      }
      size.cx=xlen;
      size.cy=ylen;
    }
    else size=pvDC->GetTextExtent(p,len);
    
    //First, check if widened rectangle is clear --
    if(TestMap(x-spcx,y-spcy,x+size.cx+spcx,y+size.cy+spcy)) return FALSE;
       
    //Now set bits for actual rectangle around label --
    SetMap(x,y,x+size.cx,y+size.cy);
    
    return TRUE;
}
#endif

bool CLayerSet::m_bPtNodeInitialized=false;
bool CLayerSet::m_bExporting=false;
double CLayerSet::m_fExportScaleRatio;
CLayerSet *CLayerSet::m_pLsetImgSource=NULL;
UINT CLayerSet::m_uScaleDenom=0;
VEC_SHPREC CLayerSet::vec_shprec_srch;
VEC_LAYER CLayerSet::vAppendLayers;

#define MAX_APPENDLAYERS (ID_APPENDLAYER_N-ID_APPENDLAYER_0) //10 for now (manually added to resource.h)

//================================================================================

int CLayerSet::GetLayerType(LPCSTR pathname)
{
	pathname=(LPCSTR)trx_Stpext(pathname);
	int i=0;
	for(;i<TYP_GDAL;i++) {
		if(!_stricmp(pathname,m_ext[i])) break;
	}
	return i;
}

const LPCSTR CLayerSet::m_ext[CLayerSet::TYP_GDAL]={".shp",".nti",".ntl"};
int CLayerSet::m_bOpenConflict=0;
bool CLayerSet::m_bReadWriteConflict=false;

int CLayerSet::IsLayer(LPCSTR lpszPathName)
{
	for(it_Layer it=m_layers.begin();it!=m_layers.end();it++) {
		ASSERT((*it)->m_pathName);
		if(!_stricmp(lpszPathName,(*it)->m_pathName))
			return m_layers.end()-it;
	}
	return 0;
}

BOOL CLayerSet::OpenLayer(LPCSTR lpszPathName,LPCSTR lpszTitle,UINT uFlags)
{
	CMapLayer *pLayer=NULL;
	int typ=GetLayerType(lpszPathName);

	m_bOpenConflict=0;

	try {
		switch(typ) {
			case TYP_SHP : if(m_layers.size() && !Layer(0).m_bTrans) {
								CMsgBox(MB_ICONINFORMATION,IDS_ERR_OPENSHAPE1,lpszPathName);
								return FALSE;
						   }
						   pLayer=new CShpLayer();
						   break;

			case TYP_NTI : pLayer=new CNtiLayer(); break;

			case TYP_NTL : CMsgBox(MB_ICONINFORMATION,IDS_ERR_OPENLSET1,lpszPathName);
						   return FALSE;

			case TYP_GDAL: pLayer=new CGdalLayer(); break;
		}
		if(pLayer) {
			pLayer->m_pDoc=m_pDoc;
			if(IsLayer(lpszPathName)) {
				CMsgBox("File %s was not added. It is already a member of this layer set.",
					trx_Stpnam(lpszPathName));
				throw 0;
			}
			ASSERT(!pLayer->IsOpen());
			//Here we should defer the actual file open if this is
			//a layerset open and if the layer isn't yet visible!
			/*
			if((uFlags&(NTL_FLG_LAYERSET|NTL_FLG_VISIBLE))==NTL_FLG_LAYERSET) {
				pLayer->SetTitle(lpszTitle);
			}
			*/
			if(pLayer->Open(lpszPathName,uFlags)) {
				pLayer->SetTitle(lpszTitle);
				if(typ==TYP_SHP) ((CShpLayer *)pLayer)->FixTitle();
				else uFlags&=~NTL_FLG_EDITABLE;
				if(InsertLayer(pLayer,uFlags))
					return TRUE;
			}
		}
		throw 0;
	}
	catch(...) {
 		delete pLayer;
	}
	return FALSE;
}

BOOL CLayerSet::InsertLayer(CMapLayer *pLayer,UINT uFlags)
{
	BOOL bRet=IsLayerCompatible(pLayer);
	//NOTE: If bRet==zero, pLayer->m_iNad is initialized to to layerset's m_iNad.

	if(bRet) {
		if(bRet>TRUE && !(uFlags&NTL_FLG_LAYERSET)) {
			if(bRet==TRUE+1) {
				//Conversion of existing view's coordinate system is required
				ASSERT(!ImageLayersPresent());
				if(IDYES!=CMsgBox(MB_YESNO,"File: %s\n\nNOTE: Addition of this image will cause the coordinate system of the view "
					"to change to that of the image, which is %s. Select YES to proceed, or NO to skip addition of this layer.",pLayer->m_pathName,
					pLayer->GetCoordSystemStr())) {
					return FALSE;
				}
			}
			else if(bRet==TRUE+2) {
				if(IDYES!=CMsgBox(MB_YESNO,"File: %s\n\nNOTE: This file has a projected coordinate system of unspecified type. Do you want to "
					"add it anyway assuming the file's coordinate system is that of the view, which is %s?",pLayer->PathName(),
					TopLayer()->GetCoordSystemStr()))
					return FALSE;
			}
			else {
				ASSERT(bRet==TRUE+3);
				if(IDYES!=CMsgBox(MB_YESNO,"File: %s\n\nNOTE: Adding this layer will change the view's assumed coordinate system to "
					"%s. Select YES to confirm change or NO to skip addition of this layer.",pLayer->PathName(),
					pLayer->GetCoordSystemStr()))
					return FALSE;
			}
		}

		if(bRet==TRUE+3) {
			ASSERT(!m_iNad || m_iZone==-99);
			m_iNad=pLayer->m_iNad;
			m_iZone=pLayer->m_iZone;
			for(rit_Layer p=m_layers.rbegin();p!=m_layers.rend();++p) {
				ASSERT(!(*p)->m_iNad || (*p)->m_iZone==-99);
				(*p)->m_iNad=m_iNad;
				(*p)->m_iZone=m_iZone;
				if((*p)->LayerType()==TYP_SHP) {
					CShpLayer *pShp=(CShpLayer *)(*p);
					ASSERT(!pShp->m_bConverted);
					pShp->m_iNadOrg=m_iNad;
					pShp->m_iZoneOrg=m_iZone;
				}
			}
		}

		if(!(uFlags&NTL_FLG_LAYERSET) && pLayer->LayerType()!=TYP_SHP) {
			rit_Layer p=m_layers.rbegin();
			for(;p!=m_layers.rend();++p) {
				if((*p)->LayerType()!=TYP_SHP) break;
			}
			m_layers.insert(p.base(),pLayer);
		}
		else m_layers.push_back(pLayer);


		if(m_layers.size()>1 &&
				(m_iZone!=pLayer->m_iZone || m_iNad!=pLayer->m_iNad)) {

			if(pLayer->LayerType()==TYP_SHP) {
				ASSERT(bRet==TRUE);
				((CShpLayer *)pLayer)->ConvertProjection(m_iNad,m_iZone,FALSE);
			}
			else {
				ASSERT(bRet>TRUE);
				//Adding an image that requires conversion of existing shapefiles and existing extent --

				if(m_iNad!=pLayer->m_iNad) {
					m_iNad=pLayer->m_iNad;
					free(m_datumName);
					if(pLayer->m_datumName) m_datumName=_strdup(pLayer->m_datumName);
					else m_datumName=NULL;
				}

				m_bProjected=pLayer->m_bProjected;
				m_iZone=pLayer->m_iZone;
				ASSERT(m_bProjected==(m_iZone!=0));

				//Update all view extents during conversion of first layer --
				BOOL bUpdateViews=TRUE;

				for(rit_Layer p=m_layers.rbegin();p!=m_layers.rend();++p) {
					if(*p!=pLayer && (*p)->LayerType()==TYP_SHP)
						bUpdateViews=((CShpLayer *)(*p))->ConvertProjection(m_iNad,m_iZone,bUpdateViews);
				}
			}
		}

		if(pLayer->LayerType()==TYP_SHP && !(uFlags&NTL_FLG_LAYERSET))
			((CShpLayer *)pLayer)->UpdateImage(m_pImageList);

		ComputeExtent();
		return TRUE;
	}
	return FALSE;
}

void CLayerSet::InitDatumNames(CMapLayer *pLayer)
{
	if(!m_datumName && pLayer->m_datumName) {
	   m_datumName=_strdup(pLayer->m_datumName);
	}
	if(!m_unitsName && pLayer->m_unitsName) {
		m_unitsName=_strdup(pLayer->m_unitsName);
	}
}

void CLayerSet::ComputeExtent()
{
	//Called after layer deletion, insertion, or replacement  --
	it_Layer p=m_layers.begin();
	if(p!=m_layers.end()) {
		CMapLayer *pLayer=*p;
		m_bShpLayers=pLayer->LayerType()==TYP_SHP;
		m_bShpPtLayers=(m_bShpLayers && 
			((CShpLayer *)pLayer)->ShpType()==CShpLayer::SHP_POINT);
		m_bTrans=pLayer->m_bTrans;
		m_bProjected=pLayer->m_bProjected;
		m_iZone=pLayer->m_iZone;
		m_iNad=pLayer->m_iNad;
		InitDatumNames(pLayer);
		while(++p!=m_layers.end()) {
			pLayer=*p;
			if(!m_bShpPtLayers && pLayer->LayerType()==TYP_SHP) {
				m_bShpLayers=true;
				m_bShpPtLayers=((CShpLayer *)pLayer)->ShpType()==CShpLayer::SHP_POINT;
			}
			ASSERT(m_bProjected==pLayer->m_bProjected && m_bTrans==pLayer->m_bTrans);
			InitDatumNames(pLayer);
			ASSERT(!m_iNad || !pLayer->m_iNad || m_iNad==pLayer->m_iNad);
			if(!m_iNad) m_iNad=pLayer->m_iNad;
			ASSERT(!m_iZone || !pLayer->m_iZone || m_iZone==pLayer->m_iZone);
			if(!m_iZone) m_iZone= pLayer->m_iZone;
		}
		GetExtent(&m_extent,FALSE); //Compute for all layers
	}
	else {
		ASSERT(FALSE);
		m_iZone=m_iNad=0;
		m_bProjected=m_bTrans=false;
		memset(&m_extent,0,sizeof(CFltRect));
	}
}

void CLayerSet::SaveShapefiles()
{
	m_nShpsEdited=0;
	for(it_Layer p=m_layers.begin();p!=m_layers.end();++p) {
		if((*p)->LayerType()==TYP_SHP && ((CShpLayer *)(*p))->m_bEditFlags) {
			if(!((CShpLayer *)(*p))->SaveShp()) m_nShpsEdited++;
		}
	}
}

bool CLayerSet::GetScaledExtent(CFltRect &fRect,double fScale)
{
	bool bInit=false;
	bool bImages=false;
	bool bVisible=fScale>=0.0;

	UINT uScaleDenom=0;

	if(fScale>0.0) {
		uScaleDenom=GetScaleDenom(fRect,fScale,IsProjected());
	}
	
	for(it_Layer p=m_layers.begin();p!=m_layers.end();p++) {
#ifdef _DEBUG
//		LPCSTR pT=(*p)->Title();
#endif
		if(!bImages) bImages=((*p)->LayerType()!=TYP_SHP);
		if((bVisible && !(*p)->IsVisible()) || (*p)->IsNullExtent() || uScaleDenom && !(*p)->LayerScaleRangeOK(uScaleDenom))
			continue;
		if(!bInit) {
			bInit=true;
			fRect=(*p)->m_extent;
		}
		else {
			const CFltRect &ext=(*p)->Extent();
			if(fRect.l>ext.l)	fRect.l=ext.l;
			if(fRect.r<ext.r)	fRect.r=ext.r;
			if(fRect.t<ext.t)	fRect.t=ext.t;
			if(fRect.b>ext.b)	fRect.b=ext.b;
		}
	}
	if(!bInit) {
		fRect=m_layers[0]->m_extent;
	}
	return bImages;
}

void CLayerSet::GetExtent(CFltRect *pfRect,BOOL bVisible)
{

	if(bVisible>1) {
		*pfRect=SelectedLayer().m_extent;
		return;
	}
	if(!bVisible && pfRect!=&m_extent) {
		*pfRect=m_extent;
		return;
	}

	bool bImages=GetScaledExtent(*pfRect,bVisible?0.0:-1.0); //ignore scale and, if <0, visibility also

	if(bVisible) return;

	//We're updating the extent due to a layer addition or deletion, or else
	//we're adding, removing, or repositioning a shapefile record. We need
	//to recompute m_fMaxImagePixelsPerGeoUnit which is used to compute
	//zoom percentages.

	m_pLayerMaxImagePixelsPerGeoUnit=m_layers[0];

	m_nteLayers.clear();

	if(!m_bTrans) {
		m_fMaxImagePixelsPerGeoUnit=1.0;
		return;
	}

	if(!bImages) {
		//Base result on shapefile with largest ratio of extent wrt corresponding screen width or height
		double maxRatio=0.0;
		HDC dc=::GetDC(NULL);
		double cx=::GetDeviceCaps(dc,HORZRES);
		double cy=::GetDeviceCaps(dc,VERTRES);
		for(it_Layer p=m_layers.begin();p!=m_layers.end();++p) {
			CFltRect ext=(*p)->Extent();
			double w=ext.Width()/cx;
			double h=ext.Height()/cy;
			if(h>maxRatio || w>maxRatio) {
				m_pLayerMaxImagePixelsPerGeoUnit=*p;
				maxRatio=max(h,w);
			}
		}
		if(maxRatio<=0.0) {
			ASSERT(0);
			maxRatio=1.0;
		}
		//return screen pixels per geounit that would fill screen with largest shapefile
		m_fMaxImagePixelsPerGeoUnit=1.0/maxRatio;
	}
	else {
		//double maxp=m_bProjected?10.0:0.0001;
		CMapLayer *max_img_l;
		double max_img=0.0;

		for(it_Layer p=m_layers.begin();p!=m_layers.end();++p) {
			CMapLayer *pLayer=*p;
			if(pLayer->LayerType()!=TYP_SHP) {

				if(pLayer->LayerType()==TYP_NTI && ((CNtiLayer *)pLayer)->HasNTERecord())
					m_nteLayers.push_back((CNtiLayer *)pLayer);

				double w=1/((CImageLayer *)pLayer)->m_fCellSize;
				if(max_img<w) {
					max_img=w;
					max_img_l=pLayer;
				}
			}
		}
		if(max_img==0.0) {
			ASSERT(0);
			max_img=1.0;
		}
		m_pLayerMaxImagePixelsPerGeoUnit=max_img_l;
		m_fMaxImagePixelsPerGeoUnit=max_img;
	}
}

UINT CLayerSet::MaxPixelWidth(const CFltRect &geoRect)
{
	UINT w=0;
	double wmin=DBL_MAX;
	for(it_Layer p=m_layers.begin();p!=m_layers.end();++p) {
		CMapLayer *pLayer=*p;
		if(pLayer->LayerType()!=TYP_SHP && pLayer->IsVisible() && pLayer->ExtentOverlap(geoRect)) {
			wmin=min(wmin,((CImageLayer *)pLayer)->m_fCellSize); //geounits per pixel
		}
	}
	if(wmin && wmin!=DBL_MAX) w=(UINT)(geoRect.Width()/wmin+0.5);
	return w;
}

void CLayerSet::InitPtNode(CDIBWrapper *pDestDIB,const CFltRect &rectGeo)
{
	int numrecs=0;
	for(it_Layer p=m_layers.begin();p!=m_layers.end();++p) {
		if((*p)->LayerType()==TYP_SHP) {
			CShpLayer *pLayer=(CShpLayer *)*p;
			if(pLayer->ShpType()==CShpLayer::SHP_POINT &&
					pLayer->IsVisible() && pLayer->ExtentOverlap(rectGeo)) {
				numrecs+=pLayer->m_nNumRecs;
			}
		}
	}
	CShpLayer::m_pPtNode->Init(pDestDIB->GetWidth(),pDestDIB->GetHeight(),numrecs);
	m_bPtNodeInitialized=true;
}

UINT CLayerSet::GetScaleDenom(const CFltRect &rectGeo,double fScale,bool bProj)
{
	//Assumes geo units are meters unless lat/long degrees!
	fScale=1.0/fScale;
	if(!bProj) {
		//multiply deg/px by meters/deg at center of view --
		double m=(rectGeo.t+rectGeo.b)/2;
		fScale *= MetricDistance(m,0,m,1)*1000;
	}
	//fScale = meters per px
	//geo-in/screen-in = fScale * 96 * (12/0.3048)
	return (UINT)(fScale*(1152/0.3048)+0.5);
}

#ifdef _USE_LYR_CULLING
static int __inline SortGridLines(VEC_FLT &vX)
{
	it_flt it0=vX.begin();
	std::sort(it0,vX.end());
	for(it_flt it=it0+1;it!=vX.end();it++)
		if(*it!=*it0) *++it0=*it;
	int size=it0-vX.begin()+1;
	ASSERT(size>=2);
	vX.resize(size);
	return size-1; //return number of intervals
}

static void RemoveHiddenImages(VEC_LAYER &vLayer,const CFltRect &rectGeo)
{
	//Partition rectGeo into rectangular regions, each one completely covered
	//by zero or more images and with no region partially covered by an image.
	//We'll use this structure to avoid rendering images that would be obscured
	//by overlying ones (those with bigger indexes in the layer array).
	//Here layers will be "removed" by setting elements of vLayer[] to NULL.

	//We're ignoring shapefiles since completely hidden shapefiles should
	//be rare and including them would only increase the number of regions tested.
	//Shapefiles would also need to be considered transparent over their extents,
	//requiring minor adjustments to the algorithm.

	try {
		VEC_FLT vCellPtX; vCellPtX.reserve(vLayer.size()*2);
		VEC_FLT vCellPtY; vCellPtY.reserve(vLayer.size()*2);

		for(it_Layer it=vLayer.begin();it!=vLayer.end();++it) {
			if((*it)->LayerType()==CLayerSet::TYP_SHP) continue; //skip
			CFltRect ext((*it)->Extent());
			VERIFY(ext.IntersectWith(rectGeo));
			vCellPtX.push_back(ext.l);
			vCellPtX.push_back(ext.r);
			vCellPtY.push_back(ext.t);
			vCellPtY.push_back(ext.b);
		}

		//Sort each grid boundary array, then remove adjacent duplicates --
		int sizeX=SortGridLines(vCellPtX); //sizeX = final number of intervals (at least 1)
		int sizeY=SortGridLines(vCellPtY);

		//Compute vector of cell centers --
		VEC_FLTPOINT vCtr;
		vCtr.reserve(sizeX*sizeY);
		//No exceptions now possible --
		for(int iy=0;iy<sizeY;iy++) {
			CFltPoint fpt;
			fpt.y=(vCellPtY[iy]+vCellPtY[iy+1])/2;
			for(int ix=0;ix<sizeX;ix++) {
				fpt.x=(vCellPtX[ix]+vCellPtX[ix+1])/2;
				vCtr.push_back(fpt);
			}
		}

		//Compute for each cell the number of occupying non-transparent image layers --
		VEC_INT vCellCount(sizeX*sizeY,0); 
		for(it_Layer p=vLayer.begin();p!=vLayer.end();++p) {
			CMapLayer *pL=*p;
			//Shapefiles and transarent layers can't obscure other layers
			//as far as we can easily determine --
			if(pL->LayerType()==CLayerSet::TYP_SHP ||
				((CImageLayer *)pL)->IsTransparent()) continue;
			for(int i=0;i<sizeX*sizeY;i++) {
				if(pL->Extent().IsPtInside(vCtr[i]))
					++vCellCount[i];
			}
		}

		//Finally remove overlapped layers --
		for(it_Layer p=vLayer.begin();p!=vLayer.end();++p) {
			CMapLayer *pL=*p;
			if(pL->LayerType()==CLayerSet::TYP_SHP) {
				continue; //Skip for now (otherwise handle a bit differently)
			}
			bool bTrans=((CImageLayer *)pL)->IsTransparent();
			UINT nCellsCovered=0,nCellsOccupied=0;
			for(int i=0;i<sizeX*sizeY;i++) {
				if(pL->Extent().IsPtInside(vCtr[i])) {
				    ASSERT(bTrans || vCellCount[i]>0);
					nCellsOccupied++;
					if(!bTrans) --vCellCount[i];
					if(vCellCount[i])
						nCellsCovered++; //Cell will be covered by a higher-indexed opaque image
				}
			}
			if(nCellsCovered==nCellsOccupied) {
				*p=NULL; //All occupied cells are covered by a higher-indexed image
				#ifdef LYR_TIMING
				nLayersSkipped++;
				#endif
			}
		}
	}
	catch(...) {
	}
}
#endif

int CLayerSet::CopyToDIB(CDIBWrapper *pDestDIB,const CFltRect &rectGeo,double fScale)
{
	//pDestDIB				- Points to screen bitmap to which image data will be copied.
	//rectGeo				- Geo coordinates corresponding to bitmap.
	//fScale				- screen pixels / geo-units.
	//
	//Returns:	0			- No data copied to bitmap.
	//			1			- Only a portion of the needed data was copied.
	//			2			- All data was copied.

	if(!ExtentOverlap(rectGeo)) {
		ASSERT(0);
		return 0;
	}

	m_uScaleDenom=GetScaleDenom(rectGeo,fScale,IsProjected());

	m_bPtNodeInitialized=false;

	VEC_LAYER vLayer;
	vLayer.reserve(NumLayers());

	UINT nImages=0;

#ifdef LYR_TIMING
	START_NTI_TM(1); //overall
#endif

	//Determine qualifying layers first since we are checking for overlaps --
	for(it_Layer p=m_layers.begin();p!=m_layers.end();++p) {
		CMapLayer *pL=*p;
		if(pL->IsVisible() && pL->LayerScaleRangeOK(m_uScaleDenom) && pL->ExtentOverlap(rectGeo)) {
			if(pL->LayerType()!=TYP_SHP) nImages++;
			vLayer.push_back(pL);
		}
	}

#ifdef _USE_LYR_CULLING
#ifdef LYR_TIMING
	START_NTI_TM(2); //culling
#endif
	//Remove any overlapped images, ignoring shapefiles since they are rarely completely
	//covered by image layers. It appears cost of this feature compared to rendering
	//time is minimal, just more code. May not be worth even that.
	if(nImages>1)
		RemoveHiddenImages(vLayer,rectGeo);

#ifdef LYR_TIMING
	STOP_NTI_TM(2); //culling
#endif
#endif

	int eRet=0,nVisible=0;
	for(it_Layer p=vLayer.begin();p!=vLayer.end();++p) {
		if(*p) {
			eRet+=(*p)->CopyToDIB(pDestDIB,rectGeo,fScale);
			nVisible++;
		}
	}

#ifdef LYR_TIMING
	STOP_NTI_TM(1); //culling
#endif

	if(HasNteLayers()) {
		//Set up CNTERecord() variables for specific scale --
		for(it_Layer p=m_nteLayers.begin();p!=m_nteLayers.end();++p) {
			ASSERT((*p)->LayerType()==TYP_NTI && ((CNtiLayer *)*p)->HasNTERecord());
			((CNtiLayer *)*p)->m_nte->SetBand(fScale);
		}
	}

	return (eRet==2*nVisible)?2:(eRet>0);
}

BOOL CLayerSet::CopyToImageDIB(CDIBWrapper *pDestDIB,const CFltRect &rectGeo)
{
	//pDestDIB				- Points to image bitmap to which data will be copied.
	//rectGeo				- Geo coordinates corresponding to bitmap.
	//Returns:	0			- No data copied to bitmap.
	//			1			- Only a portion of the data copied.
	//			2			- All data was copied.

	if(!ExtentOverlap(rectGeo)) {
		ASSERT(0);
		return TRUE;
	}
	double imgScale=pDestDIB->GetWidth()/rectGeo.Width(); //DIB pixels/geo-units
	SHP_MRK_STYLE style_saved;


	m_bPtNodeInitialized=true; //suppress initialization of CWallsMapView::m_pMapPtNode
	m_bExporting=true;

	BOOL bRet=TRUE;
	//Draw most recently added layer last --
	for(it_Layer p=m_layers.begin();p!=m_layers.end();++p) {
		if((*p)->IsVisible() && (*p)->LayerScaleRangeOK(m_uScaleDenom)) {
			if((*p)->ExtentOverlap(rectGeo)) {
				if((*p)->LayerType()==TYP_SHP) {
					style_saved=((CShpLayer *)(*p))->m_mstyle;
					((CShpLayer *)(*p))->m_mstyle.Scale(m_fExportScaleRatio); 
				}

				(*p)->CopyToDIB(pDestDIB,rectGeo,imgScale);

				if((*p)->LayerType()==TYP_SHP) {
					((CShpLayer *)(*p))->m_mstyle=style_saved;
				}
			}
		}
	}
	m_bExporting=false;

	return bRet;
}

void CLayerSet::MoveLayer(int iDst)
{
	ASSERT((UINT)iDst<m_layers.size()+1 && iDst!=m_PropStatus.nPos);
	int iSrc=SelectedLayerIt()-m_layers.begin();
	iDst=m_layers.size()-iDst;
	if(abs(iDst-iSrc)>1) {
		CMapLayer *pLayer=m_layers[iSrc];
		m_layers.erase(m_layers.begin()+iSrc);
		if(iDst>iSrc) iDst--;
		m_layers.insert(m_layers.begin()+iDst,pLayer);
	}
	else {
		std::swap(m_layers[iSrc],m_layers[iDst]);
	}
	m_PropStatus.nPos=m_layers.size()-iDst-1;
	SetChanged();
}

static LPCSTR SyncOK_str(CMapLayer *pLayer,CLayerSet *pLayerSet)
{
	return pLayer->IsSyncCompatible(pLayerSet)?" If the file is opened separately, however, synchronized views are possible.":"";
}


BOOL CLayerSet::IsLayerCompatible(CMapLayer *pLayer,BOOL bSilent /*=FALSE*/)
{
	/*
		m_iZone = 99  - unsupported projection (no conversion possible)
		m_iZone = -99 - assumed projected but projection unspecified
		m_iZone = 0   - unprojected
		else utm (m_iZone!=0 && abs(m_iZone)<=60)
		
		m_iNad = 3   - unsupported datum (no conversion possible)
		m_iNad = 0   - datum unspecified
		m_iNad = 1 or 2 - NAD83 or NAD27

	*/

	LPCSTR p=NULL;
	if(!m_layers.size()) return TRUE;

	if(!m_bTrans || !pLayer->m_bTrans) {
		if(!bSilent) CMsgBox(MB_ICONINFORMATION,IDS_ERR_OPENLAYER0,pLayer->m_pathName);
		return FALSE;
	}

	ASSERT(m_bProjected==(m_iZone!=0) && pLayer->m_bProjected==(pLayer->m_iZone!=0));

	int iNad=pLayer->m_iNad;
	int iZone=pLayer->m_iZone;

	if(!iNad && m_iNad) {
		//always assume the same datum if it's unspecified in an added layer
		iNad=m_iNad;
	}

	int iRet=TRUE;

	if(iZone==-99 && m_iZone!=-99) {
		iZone=m_iZone;
		iRet=TRUE+2;
	}

	if(m_iNad==iNad && m_iZone==iZone || 
		((!m_iNad && iNad && (m_iZone==-99 || m_iZone==iZone)) || (m_iZone==-99 && iZone!=-99 && (!m_iNad || m_iNad==iNad))) &&
		(iRet=TRUE+3))
	{
		pLayer->m_iNad=iNad;
		pLayer->m_iZone=iZone;
		if(pLayer->LayerType()==TYP_SHP) {
			((CShpLayer *)pLayer)->m_iNadOrg=iNad;
			((CShpLayer *)pLayer)->m_iZoneOrg=iZone;
		}
		return iRet;
	}

	if(iNad*m_iNad==0 || iNad>2 || m_iNad>2 || abs(iZone)>60 || abs(m_iZone)>60) {
		if(!bSilent) {
			CString s(pLayer->GetCoordSystemStr());
			CMsgBox(MB_ICONINFORMATION,
				"File: %s\n\nThis file can't be added to the current view since coordinate systems are "
			    "different and either the file or the view has an unknown or unsupported projection and/or datum.\n\n"
				"File: %s\n"
				"View: %s",
				pLayer->PathName(),(LPCSTR)s,TopLayer()->GetCoordSystemStr());
		}
		return FALSE;
	}

	//A coordinate system conversion of either view or layer will be required --

	if(pLayer->LayerType()!=TYP_SHP) {
		if(ImageLayersPresent()) {
			if(!bSilent) {
				CString s(pLayer->GetCoordSystemStr());
				CMsgBox(MB_ICONINFORMATION,"File: %s\n\nThis image can't be added as a layer since the view already "
				    "has an image with a different projection and/or datum, either specified or assumed.%s\n\n"
					"File: %s\n"
					"View: %s",
					pLayer->PathName(),SyncOK_str(pLayer,this),(LPCSTR)s,TopLayer()->GetCoordSystemStr());
			}
			return FALSE;
		}
		return TRUE+1; //convert the view
	}

    //convert the shapefile silently --
	pLayer->m_iNad=iNad;
	pLayer->m_iZone=iZone;
	if(pLayer->LayerType()==TYP_SHP) {
		((CShpLayer *)pLayer)->m_iNadOrg=iNad;
		((CShpLayer *)pLayer)->m_iZoneOrg=iZone;
	}
	return TRUE;
}

bool CLayerSet::ShpLayersVisible(UINT flg,UINT minAnd)
{
	for(rit_Layer p=m_layers.rbegin();p!=m_layers.rend();++p) {
		if((*p)->IsVisible() && (*p)->LayerType()==TYP_SHP && ((*p)->m_uFlags&flg)>minAnd)
			return true;
	}
	return false;
}

bool CLayerSet::ImageLayersPresent()
{
	for(rit_Layer p=m_layers.rbegin();p!=m_layers.rend();++p) {
		if((*p)->LayerType()!=TYP_SHP)	return true;
	}
	return false;
}

int CLayerSet::NumLayersVisible()
{
	int n=0;
	for(it_Layer p=m_layers.begin();p!=m_layers.end();++p) {
		if((*p)->IsVisible()) n++;
	}
	return n;
}

double CLayerSet::ImagePixelsPerGeoUnit(const CFltPoint &ptGeo)
{
	//return m_fTransGeo[1] if pGeoPt is within the extent of a visible image, else return -1.
	for(rit_Layer p=m_layers.rbegin();p!=m_layers.rend();++p) {
		CMapLayer *pLayer=*p;
		if(pLayer->IsVisible() && pLayer->LayerType()!=TYP_SHP && (!pLayer->IsTransformed() || pLayer->Extent().IsPtInside(ptGeo))) {
			return 1/((CImageLayer *)pLayer)->m_fCellSize;
		}
	}
	return -1.0;
}

CImageLayer * CLayerSet::GetTopImageLayer(const CFltPoint &ptGeo)
{
	for(rit_Layer p=m_layers.rbegin();p!=m_layers.rend();++p) {
		CMapLayer *pLayer=*p;
		if(pLayer->IsVisible() && pLayer->LayerType()!=TYP_SHP && (!pLayer->IsTransformed() || pLayer->Extent().IsPtInside(ptGeo))) {
			return (CImageLayer *)pLayer;
		}
	}
	return NULL;
}

CImageLayer * CLayerSet::GetTopNteLayer(const CFltPoint &ptGeo)
{
	for(rit_Layer p=m_nteLayers.rbegin();p!=m_nteLayers.rend();++p) {
		if((*p)->Extent().IsPtInside(ptGeo))
			return (CImageLayer *)*p;
	}
	return NULL;
}

void CLayerSet::CreateImageList()
{
	ASSERT(!m_pImageList);
	if(m_pImageList=new CImageList()) {
		CBitmap bm;
		VERIFY(bm.LoadBitmap(IDB_WHITE));
		VERIFY(m_pImageList->Create(20,18,ILC_COLOR24,1,1));
		VERIFY(m_pImageList->Add(&bm,(CBitmap *)NULL)==0);
	}
	ASSERT(m_pImageList);
}

void CLayerSet::DeleteLayer()
{
	ASSERT(m_layers.size()>1 && m_layers.size()>(UINT)m_PropStatus.nPos);

	it_Layer it=SelectedLayerIt();

	if(it==m_layers.begin()) m_PropStatus.nPos--;
	int n=(*it)->m_nImage;
	delete *it;
	m_layers.erase(it);
	if(n>0) {
		VERIFY(m_pImageList->Remove(n));
		for(it=m_layers.begin();it!=m_layers.end();it++) {
			ASSERT((*it)->m_nImage!=n);
			if((*it)->m_nImage>n) (*it)->m_nImage--;
		}
	}
	SetChanged();
	ComputeExtent();
}

void CLayerSet::ReplaceLayer(CMapLayer *pLayer)
{
	rit_Layer rit=SelectedLayerRit();
	pLayer->m_pDoc=m_pDoc;
	delete *rit;
	*rit=pLayer;
	if(NumLayers()>1) SetChanged();
	ComputeExtent();
}

bool CLayerSet::HasSearchableLayers()
{
	if(!m_bShpPtLayers) return FALSE;
	for(it_Layer p=m_layers.begin();p!=m_layers.end();++p) {
		if((*p)->IsSearchable() && (*p)->IsVisible()) return TRUE;
	}
	return FALSE;
}

int CLayerSet::GetAppendableLayers(CMapLayer *pShp)						   
{
	extern BOOL hPropHook;

	vAppendLayers.clear();

	if(!m_bShpPtLayers) return 0;

	UINT count=0;

	bool bFromView=(pShp==NULL);

	if(hPropHook && bFromView) {
		pShp=SelectedLayerPtr();
		if(pShp->IsVisible() && pShp->IsAppendable()) {
			vAppendLayers.push_back(pShp);
			count++;
		}
	}
	for(rit_Layer p=m_layers.rbegin();p!=m_layers.rend();++p) {
		if(*p!=pShp && (*p)->IsAppendable() && (!bFromView || (*p)->IsVisible()) &&
				(bFromView || ((CShpLayer *)pShp)->m_pdb->FieldsMatch(((CShpLayer *)*p)->m_pdb))) {
			vAppendLayers.push_back(*p);
			if(++count>=MAX_APPENDLAYERS) break;
		}
	}
	return count;
}

bool CLayerSet::FindByLabel(const CString &text,WORD wFlags)
{
	char target[128];
	strcpy(target,text);
	if(!(wFlags&F_MATCHCASE))
		SetLowerNoAccents(target);

	ASSERT(!vec_shprec_srch.size());
	vec_shprec_srch.clear();

	CWallsMapDoc::m_bSelectionLimited=false;

	CFltRect frView;
	if(wFlags&F_VIEWONLY) {
		CWallsMapView *pView=m_pDoc->GetFirstView();
		ASSERT(pView && pView->GetDocument()==m_pDoc);
		if(pView) pView->GetViewExtent(frView);
		else wFlags&=~F_VIEWONLY;
	}

	UINT ftyp=GET_FTYP(wFlags);

	if(ftyp==FTYP_ADDMATCHED || ftyp==FTYP_ADDUNMATCHED) {
		//Add to vec_shprec_srch while observing the selected flags --
	    vec_shprec_srch.reserve(app_pShowDlg->NumSelected());
		StoreSelectedRecs();
	}

	bool bModified=false;

	for(it_shpopt it=vec_shpopt.begin();it!=vec_shpopt.end();it++) {
		if(!it->bSearchExcluded && (!(wFlags&F_VIEWONLY) || extentOverlap(frView,it->pShp->m_extent)) &&
			it->pShp->GetMatches(it->pvSrchFlds,target,wFlags,frView))
		{
			bModified=true;
			if(vec_shprec_srch.size()>MAX_VEC_SHPREC_SIZE) {
				//possible only for FTYP_SELMATCHED,FTYP_SELUNMATCHED,FTYP_ADDMATCHED,FTYP_ADDUNMATCHED
				ASSERT(ftyp<FTYP_KEEPMATCHED);
				break;
			}
		}
	}

	if(bModified) {
#if 0
		if(ftyp>=FTYP_ADDMATCHED) {
			if(ftyp==FTYP_ADDMATCHED || ftyp==FTYP_ADDUNMATCHED) {
				ASSERT(m_pDoc->m_vec_shprec.size());
				//Remove already selected elements beyond argument (not needed since we tested for selection!)
				CompactSrchVecShprec(m_pDoc->m_vec_shprec.size());
				//sorting required!
			}
			else {
				//FTYP_KEEPMATCHED,FTYP_KEEPUNMATCHED
				//Only selected records are kept in vec_shprec_srch
				CompactSrchVecShprec(0); //also unnecessary!
				//no sorting required!
			}
		}
#endif
		if(m_pDoc->SelectionLimited(vec_shprec_srch.size())) {
			Empty_shprec_srch();
			return false;
		}
		if(vec_shprec_srch.size()>1 && ftyp<FTYP_KEEPMATCHED) {
			InitLayerIndices();
			CShpLayer::Sort_Vec_ShpRec(vec_shprec_srch);
		}
		return m_pDoc->ReplaceVecShprec();
	}

	if(ftyp==FTYP_ADDMATCHED || ftyp==FTYP_ADDUNMATCHED) {
		Empty_shprec_srch();
	}

	return false; //caller will optionally clear the existing selection
}

UINT CLayerSet::SelectEditedShapes(UINT uFlags,BOOL bAddToSel,CFltRect *pRectView)
{
	UINT count=0;

	if(bAddToSel) {
		ASSERT(!vec_shprec_srch.size());
		vec_shprec_srch.clear();
	    vec_shprec_srch.reserve(app_pShowDlg->NumSelected());
		StoreSelectedRecs(); //does not affect selected flags 
	}
	else {
		//clear m_pDoc->m_vec_shprec prior to first push_back --
		m_pDoc->ClearSelChangedFlags(); //prepare for UnflagSelectedRecs() within pShp->SelectEditedShapes
		bAddToSel=2;
	}

	for(it_Layer p=m_layers.begin();p!=m_layers.end();++p) {
		if((*p)->LayerType()==TYP_SHP) {
			CShpLayer *pShp=(CShpLayer *)(*p);
			//don't check layer's current extent when looking for deletions --
			//if(pShp->IsEditable() && pShp->IsVisible() && ((uFlags&SHP_EDITDEL) || !pRectView || pRectView->IntersectWith(pShp->m_extent))) {
			if(pShp->ShpType()==CShpLayer::SHP_POINT && pShp->IsVisible()) {
				if((uFlags&SHP_EDITDEL) || !pRectView || extentOverlap(*pRectView,pShp->m_extent)) {
					count+=pShp->SelectEditedShapes(uFlags,bAddToSel,pRectView);
					if(count && bAddToSel>1) bAddToSel=0;
				}
			}
		}
	}

	if(bAddToSel>1) {
		ASSERT(!count);
		bAddToSel=0;
	}

	if(count) {
		VEC_SHPREC &vec_shprec=bAddToSel?vec_shprec_srch:m_pDoc->m_vec_shprec;
		if(vec_shprec.size()>1) {
			InitLayerIndices(); //for sorting result
			CShpLayer::Sort_Vec_ShpRec(vec_shprec);
		}
		if(bAddToSel) {
			if(!m_pDoc->ReplaceVecShprec()) count=-1;
			//also freed memory in vec_shprec_srch
		}
		else {
			m_pDoc->FlagSelectedRecs();
			m_pDoc->RefreshTables();
		}
	}
	else if(bAddToSel) {
		Empty_shprec_srch();
	}

	return count;
}

void CLayerSet::InitLayerIndices()
{
	UINT n=0;
	for(it_Layer p=m_layers.begin();p!=m_layers.end();++p,n++) {
		if((*p)->LayerType()==TYP_SHP) ((CShpLayer *)(*p))->m_nLayerIndex=n;
	}
}

void CLayerSet::DrawOutlines(CDC *pDC,CFltRect &rectGeo,double fScale)
{
	CBrush cbr(RGB(0xFF,0,0));

	if(m_pDoc->m_bDrawOutlines>1) {
		it_Layer p=SelectedLayerIt();
		if((*p)->ExtentOverlap(rectGeo)) {
			(*p)->DrawOutline(pDC,rectGeo,fScale,&cbr);
		}
	}
	else
		for(it_Layer p=m_layers.begin();p!=m_layers.end();++p) {
			if((*p)->ExtentOverlap(rectGeo)) {
				(*p)->DrawOutline(pDC,rectGeo,fScale,&cbr);
			}
		}
}

#if 0
void CLayerSet::CompactSrchVecShprec(int iStart)
{
	//if iStart==0, only selected records are kept in vec_shprec_srch starting at position 0.
	//if iStart>0, only unselected records are kept in vec_shprec_srch starting at iStart.
	ASSERT(app_pShowDlg);

	VEC_SHPREC::iterator it;
	VEC_SHPREC::iterator it0=vec_shprec_srch.begin()+iStart;
	for(it=it0;it!=vec_shprec_srch.end();it++) {
		ASSERT(it->rec);
		bool bSel=it->pShp->IsRecSelected(it->rec);
		if(bSel==(iStart==0)) {
			if(it!=it0) *it0=*it;
			it0++;
		}
		else {
			ASSERT(0);
		}
	}
	if(it!=it0) {
		ASSERT(0);
		vec_shprec_srch.resize(it0-vec_shprec_srch.begin());
	}
}
#endif

void CLayerSet::StoreSelectedRecs()
{
	ASSERT(app_pShowDlg);
	ASSERT(m_bSelectedSetFlagged);

	VEC_SHPREC &vec_shprec=m_pDoc->m_vec_shprec;

	for(VEC_SHPREC::iterator it=vec_shprec.begin();it!=vec_shprec.end();it++) {
		if(it->rec) {
			vec_shprec_srch.push_back(*it);
		}
	}
}

void CLayerSet::FlagSelectedRecs()
{
	ASSERT(!m_bSelectedSetFlagged);

	VEC_SHPREC &vec_shprec=m_pDoc->m_vec_shprec;
	for(VEC_SHPREC::iterator it=vec_shprec.begin();it!=vec_shprec.end();it++) {
		if(it->rec) {
			it->pShp->SetRecSelected(it->rec);
			it->pShp->m_bSelChanged=true;
		}
	}
	m_bSelectedSetFlagged=true;
}

void CLayerSet::UnflagSelectedRecs()
{
	ASSERT(m_bSelectedSetFlagged);

	VEC_SHPREC &vec_shprec=m_pDoc->m_vec_shprec;

#ifdef _DEBUG
	UINT cnt=0;
#endif
	for(VEC_SHPREC::iterator it=vec_shprec.begin();it!=vec_shprec.end();it++) {
		if(it->rec) {
			ASSERT(++cnt);
			it->pShp->SetRecUnselected(it->rec);
			it->pShp->m_bSelChanged=true;
		}
	}
    ASSERT(!app_pShowDlg || app_pShowDlg->m_pDoc!=m_pDoc || cnt==app_pShowDlg->NumSelected());
	m_bSelectedSetFlagged=false;
}

BOOL CLayerSet::IsSetSearchable(BOOL bActive)
{
	if(bActive) {
		for(it_shpopt it=vec_shpopt.begin();it!=vec_shpopt.end();it++)
			if(!it->bSearchExcluded && it->pvSrchFlds->size()) return TRUE;
	}
	else {
		for(it_Layer it=m_layers.begin();it!=m_layers.end();it++) {
			CShpLayer *pShp=(CShpLayer *)*it;
			if(pShp->IsSearchable() && pShp->IsVisible() &&
				!pShp->IsSearchExcluded() && pShp->m_vSrchFlds.size()) return TRUE;
		}
	}
	return FALSE;
}

bool CLayerSet::InitVecShpopt(bool bAdv)
{
	bool bChanged=FALSE;
	ClearVecShpopt();
	vec_shpopt.reserve(NumLayers());
	for(rit_Layer it=BeginLayerRit();it!=EndLayerRit();it++) {
		if((*it)->IsSearchable() && (*it)->IsVisible()) {
			CShpLayer *pShp=(CShpLayer *)*it;
			vec_shpopt.push_back(SHPOPT(pShp,bAdv));
			if(bAdv && !bChanged)
				bChanged=(pShp->m_pvSrchFlds!=&pShp->m_vSrchFlds || pShp->m_bSearchExcluded!=pShp->IsSearchExcluded());
		}
	}
	return bChanged;
}

void CLayerSet::ClearSelChangedFlags()
{
	for(it_Layer it=BeginLayerIt();it!=EndLayerIt();it++) {
		if((*it)->IsSearchable()) {
			//This is a point shapefile
			((CShpLayer *)*it)->m_bSelChanged=false;
		}
	}
}

void CLayerSet::RefreshTables()
{
	for(it_Layer it=BeginLayerIt();it!=EndLayerIt();it++) {
		if((*it)->IsSearchable()) {
			CShpLayer *pShp=(CShpLayer *)*it;
			if(pShp->m_bSelChanged && pShp->IsGridDlgOpen())
				pShp->m_pdbfile->pDBGridDlg->RefreshTable();
		}
	}
}

int CLayerSet::GetUTMOtherDatum(CFltPoint &fpt)
{
	if(m_iZone) {
		//Coordinates are UTM
		//Need to convert to Lat/Long before datum change --
		geo_UTM2LatLon(m_iZone,fpt.x,fpt.y,&fpt.y,&fpt.x,(m_iNad==2)?geo_NAD27_datum:geo_WGS84_datum);

	}
	geo_FromToWGS84(m_iNad==1,&fpt.y,&fpt.x,(m_iNad==1)?geo_NAD27_datum:geo_WGS84_datum);
	int iZone=0;
	geo_LatLon2UTM(fpt.y,fpt.x,&iZone,&fpt.x,&fpt.y,(m_iNad==1)?geo_NAD27_datum:geo_WGS84_datum);
	return iZone;
}

void CLayerSet::GetDEGOtherDatum(CFltPoint &fpt)
{
	if(m_iZone) {
		//Coordinates are UTM
		//Need to convert to Lat/Long before datum change --
		geo_UTM2LatLon(m_iZone,fpt.x,fpt.y,&fpt.y,&fpt.x,(m_iNad==2)?geo_NAD27_datum:geo_WGS84_datum);
	}
	geo_FromToWGS84(m_iNad==1,&fpt.y,&fpt.x,(m_iNad==1)?geo_NAD27_datum:geo_WGS84_datum);
}

void CLayerSet::GetOtherDatumName(CString &name)
{
	if(m_pDoc->IsGeoRefSupported()) {
		if(m_iNad==1) name="NAD27";
		else name="WGS84";
	}
}

void CLayerSet::PrepareLaunchPt(CFltPoint &fpt)
{
	if(m_iNad!=1 || m_iZone) {
		int src_datum=(m_iNad==2)?geo_NAD27_datum:geo_WGS84_datum;
		if(m_iZone) {
			//Convert UTM to Lat/Long --
			geo_UTM2LatLon(m_iZone,fpt.x,fpt.y,&fpt.y,&fpt.x,src_datum);
		}
		//and switch datums if required --
		if(src_datum!=geo_WGS84_datum) {
			geo_FromToWGS84(false,&fpt.y,&fpt.x,src_datum);
		}
	}
}

HMENU CLayerSet::AppendCopyMenu(CMapLayer *pLayer,CMenu *pPopup,LPCSTR pItemName)
{
	if(!pLayer || !GetAppendableLayers(pLayer))
		return NULL;
	HMENU hPopup=CreatePopupMenu();
	if(hPopup) {
		CString s;
		s.Format("Copy %s to layer...",pItemName);
		UINT id=ID_APPENDLAYER_0;
		for(VEC_LAYER::const_iterator p=vAppendLayers.begin();p!=vAppendLayers.end();p++,id++) {
			VERIFY(AppendMenu(hPopup,MF_STRING,id,(LPCTSTR)(*p)->Title()));
		}
		VERIFY(pPopup->AppendMenu(MF_POPUP,(UINT_PTR)hPopup,(LPCSTR)s));
	}
	return hPopup;
}
