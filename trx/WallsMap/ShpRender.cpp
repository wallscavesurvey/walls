#include "stdafx.h"
#include "WallsMap.h"
#include "mainfrm.h"
#include "resource.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "ShpLayer.h"
#include "ShowShapeDlg.h"
#ifdef _USE_QTFLG
#include "QuadTree.h"
#endif

//In centroid.cpp --
BOOL PolyCentroid(LPPOINT pCentroid,LPPOINT points, LPINT pnPoints,int nPolys);
BOOL PolyCentroidF(CD2DPointF &pCentroid,CD2DPointF *points, LPINT pnPoints,int nPolys);

#ifdef _USE_CRO
#define CAIRO_WIN32_STATIC_BUILD
#include <../cairo/src/cairo-win32.h>
#ifdef _DEBUG
#pragma comment(lib,"cairoD")
#pragma comment(lib,"pixmanD")
#else
#pragma comment(lib,"cairo")
#pragma comment(lib,"pixman")
#endif
#endif

#undef _USE_PGCLIPPING
#undef _USE_PLCLIPPING

//Would allow shifted labeling of polygons partially in view --
#ifdef _USE_PGCLIPPING
#include "PolyGonClip.h"
#endif

static int lblFldLen;
static int lblHeight;
static int lblOffset;

static double clipXmin,clipXmax,clipYmin,clipYmax,clipScale;

static __inline void SetClipRange(const CFltRect &rect, const double &scale)
{
	clipXmin=rect.l;
	clipXmax=rect.r;
	clipYmin=rect.b;
	clipYmax=rect.t;
	clipScale=scale;
}

static __inline CD2DPointF ClippedPtF(const CFltPoint &p)
{
	return CD2DPointF((float)(int)(clipScale*(p.x - clipXmin)) + 0.5f,
		              (float)(int)(clipScale*(clipYmax - p.y)) + 0.5f);
}

static void PrintLabel(CDC *pDC,CPoint &pt,LPCSTR pStart)
{
	int len=lblFldLen;
	while(len && *pStart==' ') {len--; pStart++;}
	while(len && pStart[len-1]==' ') len--;
	if(len) ::TextOut(pDC->GetSafeHdc(),pt.x,pt.y,pStart,len);
}

bool CShpLayer::InitTextLabels(CDC *pDC,BOOL bInit)
{
	HDC hdc=pDC->GetSafeHdc();
	if(bInit) {
		if(CLayerSet::m_bExporting) {
			long lfHeight=m_font.lf.lfHeight;
			m_font.lf.lfHeight=(long)(CLayerSet::m_fExportScaleRatio*m_font.lf.lfHeight);
			BOOL ok=InitLabelDC(hdc,&m_font,m_crLbl);
			m_font.lf.lfHeight=lfHeight;
			if(!ok) return false;
		}
		else {
			if(!InitLabelDC(hdc,&m_font,m_crLbl)) return false;
		}
		lblFldLen=m_pdb->FldLen(m_nLblFld);
		lblOffset=m_pdb->FldOffset(m_nLblFld);
		TEXTMETRIC tm;
		::GetTextMetrics(hdc,&tm);
		lblHeight=tm.tmHeight/2;
	}
	else InitLabelDC(hdc);
	return true;
}

HPEN CreateStylePen(const SHP_MRK_STYLE &style,BOOL bNonWhite)
{
	DWORD dwPenStyle=PS_INSIDEFRAME|PS_GEOMETRIC|PS_ENDCAP_FLAT|PS_JOIN_MITER;
	//Small circles look terrible otherwise --
	if(style.wShape==FST_CIRCLES) dwPenStyle=PS_GEOMETRIC;

	LOGBRUSH lb;
	DWORD pw=bNonWhite?1:(DWORD)(style.fLineWidth+0.5);
	bNonWhite=bNonWhite && style.crMrk==RGB(255,255,255); 
	lb.lbColor=bNonWhite?RGB(229,229,229):style.crMrk;
	lb.lbStyle=BS_SOLID;
	HPEN hPen=(HPEN)::ExtCreatePen(dwPenStyle,pw,&lb,0,NULL);
	if(!hPen)
		hPen=(HPEN)::GetStockObject(BLACK_PEN);
	return hPen;
}

static HBRUSH CreateStyleBrush(const SHP_MRK_STYLE &style)
{
	HBRUSH hBrush=NULL;
	if (style.wFilled&FSF_FILLED) hBrush = ::CreateSolidBrush(style.crBkg);
	if(!hBrush)
		hBrush=(HBRUSH)::GetStockObject(NULL_BRUSH);
	return hBrush;
}

#ifdef _USE_PLCLIPPING
enum e_outcode {O_LEFT=1,O_RIGHT=2,O_BOTTOM=4,O_TOP=8};

static UINT OutCode(double x,double y)
{
	UINT code=0;
	if(x<clipXmin) code=O_LEFT;
	else if(x>clipXmax) code=O_RIGHT;
	if(y<clipYmin) code |= O_TOP;
	else if(y>clipYmax) code |= O_BOTTOM;
	return code;
}

static bool ClipEndpoint(const double &x0,const double &y0,double &x1,double &y1,UINT code)
{
  //If necessary, adjusts x1,y1 to point on frame without changing slope.
  //Returns TRUE if successful (line intersects frame), otherwise FALSE.
  //Assumes code==OutCode(x1,y1) and that OutCode(x0,y0)&code==0;

  double slope;

  ASSERT((OutCode(x0,y0)&code)==0);

  if(code&(O_RIGHT|O_LEFT)) {
	slope=(y1-y0)/(x1-x0);
    x1=((code&O_RIGHT)?clipXmax:clipXmin);
    y1=y0+slope*(x1-x0);
	if(y1<clipYmin) code=O_TOP;
	else if(y1>clipYmax) code=O_BOTTOM;
	else code=0;
  }

  if(code&(O_TOP|O_BOTTOM)) {
	slope=(x1-x0)/(y1-y0);
    y1=((code&O_BOTTOM)?clipYmax:clipYmin);
    x1=x0+slope*(y1-y0);
	if(x1<clipXmin) code=O_LEFT;
	else if(x1>clipXmax) code=O_RIGHT;
	else code=0;
  }

  return code==0;
}
#endif

static __inline int _poly_compact(CPoint *p,int len)
{
	CPoint *p0=p;
	CPoint *pLim=p+len;
	while(++p<pLim) {
		if(p->x!=p0->x || p->y!=p0->y) *++p0=*p;
		else --len;
	}
	return len;
}

#ifdef _USE_CRO
static __inline void cro_Polygon(cairo_t *cr, LPPOINT ppt, int np)
{
	cairo_move_to(cr, ppt->x + 0.5, ppt->y + 0.5);
	for (LPPOINT ppx = ppt + np - 1; ++ppt<ppx;) {
		cairo_line_to(cr, ppt->x + 0.5, ppt->y + 0.5);
	}
	cairo_close_path(cr);
}

static __inline void cro_Polyline(cairo_t *cr, LPPOINT ppt, int np)
{
	cairo_move_to(cr, ppt->x+0.5, ppt->y+0.5);
	for (LPPOINT ppx = ppt + np; ++ppt<ppx;) {
		cairo_line_to(cr, ppt->x + 0.5, ppt->y + 0.5);
	}
	//cairo_close_path(cr);
}
#endif

static __inline ID2D1PathGeometry * OpenPolyPath(ID2D1GeometrySink **ppSink)
{
	ID2D1PathGeometry *pPath = NULL;
	if (0 > d2d_pFactory->CreatePathGeometry(&pPath)) return NULL;
	if (0 >= pPath->Open(ppSink)) return pPath;
	pPath->Release();
	return NULL;
}


int CShpLayer::CopyPolylinesToDIB(CDIBWrapper *pDestDIB,const CFltRect &geoExt,double fScale)
{
	//pDestDIB				- Points to screen bitmap to which image data will be copied.
	//geoExt				- Geographical extent corresponding to bitmap.
	//fScale				- Screen pixels / geo-units.
	//
	//Returns:	0			- No data copied to bitmap.
	//			1			- Only a portion of the data copied.
	//			2			- All data copied.

	if (!m_nNumRecs || !m_mstyle.fLineWidth || !m_mstyle.iOpacityVec) return 2;

	SHP_POLY_DATA *ppoly=Init_ppoly();
	SHP_POLY_DATA *pp = ppoly;

	if (!pp) return 2;

	bool bLabeling=(m_uFlags&((m_pDoc->m_uEnableFlags&NTL_FLG_LABELS)|NTL_FLG_SHOWLABELS))!=0 &&
		LabelScaleRangeOK(CLayerSet::m_uScaleDenom);

	CDC *pDC=NULL;
	HPEN hPen=NULL, hPenOld=NULL;
	bool bInsideFrm=geoExt.ContainsRect(m_extent);
#ifdef _USE_QTFLG
	BYTE *pFlg=(m_bQTusing && !bInsideFrame)?m_pQT->InitFlags(geoExt):NULL;
#endif
#ifdef _USE_CRO
	cairo_surface_t *cs=NULL;
	cairo_t *cr=NULL;
#else
	ID2D1DCRenderTarget *pRT = NULL; //will call pRT=pDestDIB->InitRT()
	ID2D1SolidColorBrush *pBR = NULL; //will call pBR=pDestDIB->InitBR()
	ID2D1PathGeometry *pPath = NULL;
	ID2D1GeometrySink *pSink = NULL;
#endif
	
	SetClipRange(geoExt,fScale);

	bool bD2D = (m_mstyle.wFilled&FSF_USED2D) != 0;
	bool bConvert = m_bConverted && (m_iZone != m_iZoneOrg || m_iNad != m_iNadOrg);
	VEC_LBLPOS vLblPos;

	if (bLabeling) {
		//reserve space for label locations --
		UINT sz = 0;
		if(bInsideFrm) sz=m_nNumRecs;
		else {
            #ifdef _USE_QTFLG
			if(pFlg) {
				BYTE *pF=pFlg+m_nNumRecs;
				for(BYTE *p=pFlg;p<pF;p++) sz+=*p;
			}
		    else {
				for (it_rect it = m_ext.begin(); it != m_ext.end(); it++, pp++) {
					if (!pp->IsDeleted() && !geoExt.IsRectOutside(*it)) sz++;
				}
			    pp = ppoly;
			}
		    #else
			for (it_rect it = m_ext.begin(); it != m_ext.end(); it++, pp++) {
				if (!pp->IsDeleted() && !geoExt.IsRectOutside(*it)) sz++;
			}
			pp = ppoly;
            #endif
		}
		vLblPos.reserve(sz);
	}

	CFileMap &cf=*m_pdbfile->pfShp;
	VEC_CPOINT vp;
    UINT nRecsValid=0,nRecsVisible=0;
	
	START_NTI_TM(0);

	try {
		for(it_rect it_ext=m_ext.begin();it_ext!=m_ext.end();it_ext++,pp++) {
            #ifdef _USE_QTFLG
			if(pFlg) {
			    ASSERT((*pFlg==0)==(pp->IsDeleted() || geoExt.IsRectOutside(*it_ext)));
				if(!*pFlg++) continue;
			}
			else
			#endif
			if(pp->IsDeleted() || !bInsideFrm && geoExt.IsRectOutside(*it_ext))
				continue;
	
			nRecsValid++;

			int *parts = (int *)m_pdbfile->pbuf;

			if (!parts || pp->Len() > m_pdbfile->pbuflen) {
				parts = (int *)Realloc_pbuf(pp->Len());
				if (!parts) throw 0;
			}

			cf.Seek(pp->off, CFile::begin);
			if (cf.Read(parts, pp->Len()) != pp->Len()) throw 0; //error

			int iLastPart = *parts++; //numParts
			int numPoints = *parts++; //numPoints
			ASSERT(numPoints);

			//Create geometry (possibly clipped) --

			CFltPoint *pt = (CFltPoint *)&parts[iLastPart--]; //first point

			if (bConvert)
				ConvertPointsFromOrg((CFltPoint *)pt, numPoints);

			if (!pDC) {
				VERIFY(pDC = pDestDIB->GetMemoryDC());
				if (!pDC) return 2;

				if (bD2D) {
					#ifdef _USE_CRO
					cs = cairo_win32_surface_create(pDC->GetSafeHdc());
					cr = cairo_create(cs);
					cairo_set_antialias(cr, (m_mstyle.wFilled&FSF_ANTIALIAS) ? CAIRO_ANTIALIAS_DEFAULT : CAIRO_ANTIALIAS_NONE);
					cairo_set_line_width(cr, m_mstyle.fLineWidth);
					cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
					cairo_set_source_rgba(cr,GetR_f(m_mstyle.crMrk),GetG_f(m_mstyle.crMrk),	GetB_f(m_mstyle.crMrk),	m_mstyle.iOpacityVec*0.01);
					#else
					pRT = pDestDIB->InitRT();
					if (!pRT) {
						ASSERT(0);
						return 2;
					}
					pRT->SetAntialiasMode((m_mstyle.wFilled&FSF_ANTIALIAS) ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
					pBR = pDestDIB->InitBR(m_mstyle.crMrk, m_mstyle.iOpacityVec*0.01f);
					if (!pBR) {
						ASSERT(0);
						return 2;
					}
					pRT->BeginDraw();
					#endif
				}
				else {
					hPen = CreateStylePen(m_mstyle);
					hPenOld = (HPEN)::SelectObject(pDC->GetSafeHdc(), hPen);
				}
			}

#ifndef _USE_CRO
			//Separarate geometry for each polygon -- faster and requires much less peak ram --
			if (bD2D && !(pPath = OpenPolyPath(&pSink))) {
				ASSERT(0);
				break;
			}
#endif

#ifdef _USE_PLCLIPPING
			//Currently not working -- excludes lines or line segments
			if(!geoExt.IsRectInside(*it_ext)) {
				//polyline not entirely inside DIB, clipping could help with determining label position and may guard against float overflow.
				//evidently not significantly faster!!

				UINT uClip0, uClip1;
				double x0, y0, x1, y1;
				int ix, ix0, iy, iy0;

				for(int iPart=0;iPart<=iLastPart;iPart++) {
					int iLimit=(iPart<iLastPart)?parts[iPart+1]:numPoints; ///one past index of last pt in this part
					int i=parts[iPart]; //index of first pt in this part
					double *p=&pt[i].x;
					bool bBeginFigure = false;

					x0=*p++; y0=*p++;
					if(!(uClip0=OutCode(x0,y0))) {
						//First point is inside window. Cairo and pDC will Move to (ix0,iy0)
						if (bD2D) {
							ix0 = ix = (int)(clipScale*(x0 - clipXmin));
							iy0 = iy = (int)(clipScale*(clipYmax - y0));
							#ifdef _USE_CRO
							cairo_move_to(cr,ix+0.5,iy+0.5);
							#else
							pSink->BeginFigure(D2D1::Point2F(ix+0.5f,iy+0.5f), D2D1_FIGURE_BEGIN_HOLLOW);
							bBeginFigure = true;
							#endif
						}
						else {
							ix0 = ix = (int)(clipScale*(x0 - clipXmin)+0.5);
							iy0 = iy = (int)(clipScale*(clipYmax - y0)+0.5);
							pDC->MoveTo(ix, iy);
						}
					}
					while(++i<iLimit) {
						x1=*p++; y1=*p++;
						uClip1=OutCode(x1,y1);
						if(!(uClip0 & uClip1)) {
							//either both inside (uClip0=clip1=0) or in different regions (9 regions total incl center)
							if(uClip0) {
								//Point (x0,y0) was outside frame and (x1,y1) is in a different region -- inside frame if uClip1=0.
								//Change it to point where line enters frame and move to it
								if(!ClipEndpoint(x1,y1,x0,y0,uClip0)) goto _next;
								if (bD2D) {
									ix0 = ix = (int)(clipScale*(x0 - clipXmin));
									iy0 = iy = (int)(clipScale*(clipYmax - y0));
									#ifdef _USE_CRO
									cairo_move_to(cr, ix+0.5, iy+0.5);
									#else
									if (bBeginFigure) {
										pSink->EndFigure(D2D1_FIGURE_END_OPEN);
									}
									pSink->BeginFigure(D2D1::Point2F(ix+0.5f, iy+0.5f), D2D1_FIGURE_BEGIN_HOLLOW);
									bBeginFigure = true;
									#endif
								}
								else {
									ix0 = ix = (int)(clipScale*(x0 - clipXmin)+0.5);
									iy0 = iy = (int)(clipScale*(clipYmax - y0)+0.5);
									pDC->MoveTo(ix, iy);
								}
							}
							ASSERT(!OutCode(x0,y0));
							if(!uClip1 || ClipEndpoint(x0,y0,x1,y1,uClip1)) {
								if (ix != ix0 || iy != iy0) {
									if (bD2D) {
										ix=(int)(clipScale*(x1-clipXmin));
										iy=(int)(clipScale*(clipYmax-y1));
										#ifdef _USE_CRO
										cairo_line_to(cr, (ix0 = ix)+0.5, (iy0 = iy)+0.5);
										#else
										ASSERT(bBeginFigure);
										pSink->AddLine(D2D1::Point2F((ix0 = ix) + 0.5f, (iy0 = iy) + 0.5f));
										#endif
									}
									else {
										ix = (int)(clipScale*(x1 - clipXmin)+0.5);
										iy = (int)(clipScale*(clipYmax - y1)+0.5);
										pDC->LineTo(ix0 = ix, iy0 = iy);
									}
								}
							}
						}
					_next:
						uClip0=uClip1;
						x0=x1; y0=y1;
					}
					#ifndef _USE_CRO
					if (bBeginFigure) {
						pSink->EndFigure(D2D1_FIGURE_END_OPEN);
					}
					#endif
				}
			}
			else
				//No clipping required --
#endif			
			{

				#ifdef _USE_CRO
					CFltPoint *p = pt;
					vp.clear();
					vp.reserve(numPoints);
					double fInc = bD2D ? 0.0 : 0.5;
					for (int ix = numPoints; ix--; p++) {
						vp.push_back(CPoint((int)(clipScale*(p->x - clipXmin)+fInc), (int)(clipScale*(clipYmax - p->y)+fInc)));
					}

					//Draw polyine parts --
					int *ipLast = parts + iLastPart;
					for (int *ip = parts; ip <= ipLast; ip++) {
						int ix = ((ip<ipLast) ? ip[1] : numPoints) - *ip; //no. of points in this part
						CPoint *cp = &vp[*ip];
						if ((ix = _poly_compact(cp, ix))>1)
						if (bD2D)
							cro_Polyline(cr, cp, ix);
						else
							pDC->Polyline(cp, ix);
					}

					if (bD2D) cairo_stroke(cr);
				#else
					//Draw all polyline parts --
					CFltPoint *p = pt;
					if (!bD2D) {
						vp.clear();
						vp.reserve(numPoints);
						for (int ix = numPoints; ix--; p++) {
							vp.push_back(CPoint((int)(clipScale*(p->x - clipXmin) + 0.5), (int)(clipScale*(clipYmax - p->y) + 0.5)));
						}
						p = pt;
					}

					int *ipLast = parts + iLastPart;

					for (int *ip = parts; ip <= ipLast; ip++) {
						int ix = ((ip < ipLast) ? ip[1] : numPoints) - *ip; //no. of points in this part
						ASSERT(ix>1);
						if (ix < 2) continue;
						if (bD2D) {
							pSink->BeginFigure(ClippedPtF(*p), D2D1_FIGURE_BEGIN_HOLLOW);
							for (++p; --ix; p++) {
								pSink->AddLine(ClippedPtF(*p));
							}
							pSink->EndFigure(D2D1_FIGURE_END_OPEN);
						}
						else {
							CPoint *cp = &vp[*ip];
							if ((ix = _poly_compact(cp, ix))>1) {
								pDC->Polyline(cp, ix);
							}
						}
					}
				#endif
			}

#ifndef _USE_CRO
			if (bD2D) {
				pSink->Close();
				pSink->Release(); pSink = NULL;
				pRT->DrawGeometry(pPath, pBR, m_mstyle.fLineWidth, d2d_pStrokeStyleRnd);
				pPath->Release(); pPath = NULL;
			}
			//if(bD2D) VERIFY(0<=pRT->Flush()); //SLOWER
			//if (bD2D) {pRT->EndDraw();pRT->BeginDraw();} //SLOWEST
#endif

			if (bLabeling && geoExt.IsPtInside(pt[numPoints / 2]) && !m_pdb->Go((pp - ppoly) + 1)) {
				CFltPoint fp(pt[numPoints / 2]);
				CPoint cp((int)(fScale*(fp.x - geoExt.l)), (int)(fScale*(geoExt.t - fp.y)));
				vLblPos.push_back(SHP_LBLPOS(cp, pp - ppoly + 1));
			}
			nRecsVisible++;

		} //polyline record loop
	}
	catch(...) {
		ASSERT(0);
	}

	if (pDC) {
		if (bD2D) {
#ifdef _USE_CRO
			cairo_destroy(cr);
			cairo_surface_destroy(cs);
#else
			ASSERT(!pSink && !pPath);
			SafeRelease(&pSink);
			SafeRelease(&pPath);
			if (0 > pRT->EndDraw()) {
				ASSERT(0);
			}
#endif
		}
		else if (hPenOld) {
			::SelectObject(pDC->GetSafeHdc(), hPenOld);
			::DeleteObject(hPen);
		}

		STOP_NTI_TM(0);

		if (bLabeling && InitTextLabels(pDC, TRUE)) {
			::SetTextAlign(pDC->GetSafeHdc(), TA_TOP | TA_CENTER | TA_NOUPDATECP);
			for (it_lblpos it = vLblPos.begin(); it != vLblPos.end(); it++) {
				if (!m_pdb->Go(it->rec))
					PrintLabel(pDC, it->pos, (LPCSTR)m_pdb->RecPtr() + lblOffset);
			}
			InitTextLabels(pDC, FALSE);
		}
	}

	return (nRecsVisible==nRecsValid)?2:(nRecsVisible>0);
}

int CShpLayer::CopyPolygonsToDIB(CDIBWrapper *pDestDIB,const CFltRect &geoExt,double fScale)
{
	//pDestDIB				- Points to screen bitmap to which image data will be copied.
	//geoExt				- Geographical extent corresponding to bitmap.
	//fScale				- Screen pixels / geo-units.
	//
	//Returns:	0			- No data copied to bitmap.
	//			1			- Only a portion of the data copied.
	//			2			- All data copied.

	if (!m_nNumRecs || !((m_mstyle.wFilled&FSF_FILLED) && m_mstyle.iOpacitySym) && !(m_mstyle.fLineWidth && m_mstyle.iOpacityVec))
		return 2;

	SHP_POLY_DATA *ppoly=Init_ppoly();
	if(!ppoly) return 2;
	SHP_POLY_DATA *pp = ppoly;

	ASSERT(m_ext.size()==m_nNumRecs);

	CDC *pDC=NULL;

#ifdef _USE_CRO
	cairo_surface_t *cs = NULL;
	cairo_t *cr = NULL;
	double mred=0,mgreen=0,mblue=0,fred=0,fgreen=0,fblue=0;
#else
	ID2D1DCRenderTarget *pRT = NULL; //will call pRT=pDestDIB->InitRT()
	ID2D1SolidColorBrush *pBR = NULL; //will call pBR=pDestDIB->InitBR()
	ID2D1SolidColorBrush *pBRF = NULL;
	ID2D1PathGeometry *pPath = NULL;
	ID2D1GeometrySink *pSink = NULL;
#endif

	bool bLabeling=(m_uFlags&((m_pDoc->m_uEnableFlags&NTL_FLG_LABELS)|NTL_FLG_SHOWLABELS))!=0 &&
		LabelScaleRangeOK(CLayerSet::m_uScaleDenom);

	bool bD2D = (m_mstyle.wFilled&FSF_USED2D) != 0;
	HPEN hPen = NULL, hPenOld = NULL;
	HBRUSH hBrush=NULL,hBrushOld=NULL;
	VEC_CPOINT vPolyPt; 

	VEC_LBLPOS vLblPos;
	VEC_INT vPolyCnt;
	CRect clipRect;

	bool bInsideFrm=geoExt.ContainsRect(m_extent);
	#ifdef _USE_QTFLG
		BYTE *pFlg=(m_bQTusing && !bInsideFrame)?m_pQT->InitFlags(geoExt):NULL;
	#endif

	if (bLabeling) {
		//Rect that must contain centroid --
		clipRect=CRect(0, 0, //left,top,right,bottom
			(long)(fScale*(geoExt.r - geoExt.l)),
			(long)(fScale*(geoExt.t - geoExt.b)));

		//reserve space for label locations --
		UINT sz = 0;
		if(bInsideFrm) sz=m_nNumRecs;
		else {
            #ifdef _USE_QTFLG
			if(pFlg) {
				BYTE *pF=pFlg+m_nNumRecs;
				for(BYTE *p=pFlg;p<pF;p++) sz+=*p;
			}
		    else {
				for (it_rect it = m_ext.begin(); it != m_ext.end(); it++, pp++) {
					if (!pp->IsDeleted() && !geoExt.IsRectOutside(*it)) sz++;
				}
			    pp = ppoly;
			}
		    #else
			for (it_rect it = m_ext.begin(); it != m_ext.end(); it++, pp++) {
				if (!pp->IsDeleted() && !geoExt.IsRectOutside(*it)) sz++;
			}
			pp = ppoly;
            #endif
		}
		vLblPos.reserve(sz);
	}

	SetClipRange(geoExt,fScale);

	CFileMap &cf=*m_pdbfile->pfShp;
	UINT nRecsValid = 0, nRecsVisible = 0;

	START_NTI_TM(6);

	try {
		for(it_rect it_ext=m_ext.begin();it_ext!=m_ext.end();it_ext++,pp++) {
            #ifdef _USE_QTFLG
			if(pFlg) {
				ASSERT((*pFlg==0)==(pp->IsDeleted() || geoExt.IsRectOutside(*it_ext)));
				if(!*pFlg++) continue;
			}
			else
			#endif
			if(pp->IsDeleted() || !bInsideFrm && geoExt.IsRectOutside(*it_ext))
				continue;

			nRecsValid++;

			int *parts=(int *)m_pdbfile->pbuf;

			if(!parts || pp->Len()>m_pdbfile->pbuflen) {
				parts=(int *)Realloc_pbuf(pp->Len());
				if(!parts) throw 0;
			}

			cf.Seek(pp->off,CFile::begin);
			if(cf.Read(parts,pp->Len())!=pp->Len()) throw 0; //error

			int iLastPart=*parts++; //numParts
			int numPoints=*parts++; //numPoints

			vPolyPt.clear(); vPolyPt.reserve(numPoints);
			vPolyCnt.clear(); vPolyCnt.reserve(iLastPart);

			CFltPoint *pt = (CFltPoint *)&parts[iLastPart--];

			ASSERT(parts[0] == 0);
			if (m_bConverted && (m_iZone != m_iZoneOrg || m_iNad != m_iNadOrg))
				ConvertPointsFromOrg((CFltPoint *)pt, numPoints);

			if (!pDC) {
				//Initialize drawing resources --
				VERIFY(pDC = pDestDIB->GetMemoryDC());
				if (!pDC) break;

				if (bD2D) {

					#ifdef _USE_CRO
					cs = cairo_win32_surface_create(pDC->GetSafeHdc());
					cr = cairo_create(cs);
					cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD); //same as qgis but shouldn't matter for clean polys
					//cairo_set_fill_rule(cr,CAIRO_FILL_RULE_WINDING);

					//cairo_set_antialias(cr,CAIRO_ANTIALIAS_NONE);
					//cairo_set_antialias(cr,(m_mstyle.wFilled&2)?CAIRO_ANTIALIAS_DEFAULT:CAIRO_ANTIALIAS_NONE);
					cairo_set_antialias(cr, (m_mstyle.wFilled & FSF_ANTIALIAS) ? CAIRO_ANTIALIAS_SUBPIXEL : CAIRO_ANTIALIAS_NONE);
					//cairo_set_antialias(cr,CAIRO_ANTIALIAS_GRAY);

					if (m_mstyle.fLineWidth) {
						cairo_set_line_width(cr, m_mstyle.fLineWidth);
						mred = GetR_f(m_mstyle.crMrk);
						mgreen = GetG_f(m_mstyle.crMrk);
						mblue = GetB_f(m_mstyle.crMrk);
					}
					if (m_mstyle.wFilled&FSF_FILLED) {
						fred = GetR_f(m_mstyle.crBkg);
						fgreen = GetG_f(m_mstyle.crBkg);
						fblue = GetB_f(m_mstyle.crBkg);
					}
					#else

					//D2D ---
					if (!(pRT = pDestDIB->InitRT())) {
						ASSERT(0);
						return 2;
					}
					pRT->SetAntialiasMode((m_mstyle.wFilled&FSF_ANTIALIAS) ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);

					if (m_mstyle.fLineWidth && m_mstyle.iOpacityVec) {
						VERIFY(pBR = pDestDIB->InitBR(m_mstyle.crMrk, m_mstyle.iOpacityVec / 100.f));
					}
					if ((m_mstyle.wFilled&FSF_FILLED) && m_mstyle.iOpacitySym) {
						VERIFY(0 <= pRT->CreateSolidColorBrush(RGBAtoD2D(m_mstyle.crBkg, m_mstyle.iOpacitySym / 100.f), &pBRF));
					}
					pRT->BeginDraw();
					#endif

				}
				else { //gdi
					if (m_mstyle.fLineWidth) {
						VERIFY(hPen = CreateStylePen(m_mstyle));
						VERIFY(hPenOld = (HPEN)::SelectObject(pDC->GetSafeHdc(), hPen));
					}
					else {
						VERIFY(hPenOld = (HPEN)::SelectObject(pDC->GetSafeHdc(), ::GetStockObject(NULL_PEN)));
					}
					VERIFY(hBrush = CreateStyleBrush(m_mstyle));
					VERIFY(hBrushOld = (HBRUSH)::SelectObject(pDC->GetSafeHdc(), hBrush));
					//pDC->SetPolyFillMode(WINDING); //will create filled star
					pDC->SetPolyFillMode(ALTERNATE); //correct star with pentagon hole
				}

			} //!pDC

			//Store polypolygon screen coordinates --

			for (int iPart = 0; iPart <= iLastPart; iPart++) {
				int iLimit = (iPart<iLastPart) ? parts[iPart + 1] : numPoints;
				int i = parts[iPart];
				vPolyCnt.push_back(iLimit - i);
				//double perim=0.0;
				for (; i<iLimit; i++) {
					/*
					if(i>0) {
					double x2=pt[i].X-pt[i-1].X;
					double y2=pt[i].Y-pt[i-1].Y;
					perim+=sqrt(x2*x2+y2*y2);
					}
					*/
					//will add 0.5 when plotting --
					if (bD2D)
						vPolyPt.push_back(CPoint((int)(clipScale*(pt[i].x - clipXmin)), (int)(clipScale*(clipYmax - pt[i].y))));
					else
						vPolyPt.push_back(CPoint(_rnd(clipScale*(pt[i].x - clipXmin)), _rnd(clipScale*(clipYmax - pt[i].y))));
				}
				/*
				perim/=(5280.0*0.3048); //perimeter in miles
				double striparea=perim/5280.0; //area of 1-ft strip in sq miles
				double poutside=1.0-striparea/989.3; //prob of a pt not falling within the strip
				poutside=exp(616.0*log(poutside)); //prob of all pts not falling within the strip
				poutside=1.0-poutside; //prob of at least one pt falling within strip
				perim=0;
				*/
			}

			//Now draw polypolygon --
			if (iLastPart = vPolyCnt.size()) {

				LPPOINT lpp = &vPolyPt[0];
				LPINT lp = &vPolyCnt[0];

				if (bD2D) {
#ifdef _USE_CRO
					for (int i = 0; i<iLastPart; i++, lp++) {
						ASSERT(*lp >= 4);
						cro_Polygon(cr, lpp, *lp);
						lpp += *lp;
					}
					if (m_mstyle.wFilled&FSF_FILLED) {
						cairo_set_source_rgba(cr, fred, fgreen, fblue, m_mstyle.iOpacitySym*0.01);
						if (m_mstyle.fLineWidth)
							cairo_fill_preserve(cr);
						else
							cairo_fill(cr);
					}

					if (m_mstyle.fLineWidth) {
						cairo_set_source_rgba(cr, mred, mgreen, mblue, m_mstyle.iOpacityVec*0.01);
						cairo_stroke(cr);
					}
#else
					if ((pPath = OpenPolyPath(&pSink))) {
						//pSink->SetFillMode(D2D1_FILL_MODE_WINDING); //default: D2D1_FILL_MODE_ALTERNATE
						for (int i = 0; i<iLastPart; i++, lp++, lpp++) {
							ASSERT(*lp >= 4); //point count first==last
							pSink->BeginFigure(CD2DPointF(lpp->x + 0.5f, lpp->y + 0.5f), pBRF ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);
							lpp++;
							for (int ii = *lp - 1; ii>1; ii--, lpp++) pSink->AddLine(CD2DPointF(lpp->x + 0.5f, lpp->y + 0.5f));
							pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
						}
						pSink->Close();
						pSink->Release(); pSink = NULL;
						if (pBRF) pRT->FillGeometry(pPath, pBRF);
						if (pBR) pRT->DrawGeometry(pPath, pBR, m_mstyle.fLineWidth, d2d_pStrokeStyleRnd);
						pPath->Release(); pPath = NULL;
					}
#endif
				} //d2d
				else
				{
					//NOTE: PolyPolygon() is many times slower than Polygon(), but required for proper filling!
					if ((m_mstyle.wFilled&FSF_FILLED) && iLastPart>1) {
						pDC->PolyPolygon(lpp, lp, iLastPart);
					}
					else {
						for (int i = 0; i<iLastPart; i++, lp++) {
							pDC->Polygon(lpp, *lp); //*lp-1 no faster
							lpp += *lp;
						}
					}
				}

				if (bLabeling) {
					CPoint pt;
					if (PolyCentroid((LPPOINT)&pt, (LPPOINT)&vPolyPt[0], (LPINT)&vPolyCnt[0], iLastPart)) {
						if (clipRect.PtInRect(pt)) {
							vLblPos.push_back(SHP_LBLPOS(pt, pp - ppoly + 1));
						}
					}
				}

				nRecsVisible++;
			}
		} //polygon loop
	}
	catch(...) {
		ASSERT(0);
	}

	if(pDC) {

		if (bD2D) {

#ifdef _USE_CRO
			cairo_destroy(cr);
			cairo_surface_destroy(cs);
#else
			if (0 > pRT->EndDraw()) {
				ASSERT(0);
			}
#endif
		}
		else {
			if (hBrushOld) ::DeleteObject(::SelectObject(pDC->GetSafeHdc(), hBrushOld));
			if (hPenOld) ::DeleteObject(::SelectObject(pDC->GetSafeHdc(), hPenOld));
		}

		if (bLabeling && InitTextLabels(pDC, TRUE)) {
			::SetTextAlign(pDC->GetSafeHdc(), TA_TOP | TA_CENTER | TA_NOUPDATECP);
			for (it_lblpos it = vLblPos.begin(); it != vLblPos.end(); it++) {
				if (!m_pdb->Go(it->rec)) {
					it->pos.y -= lblHeight;
					PrintLabel(pDC, it->pos, (LPCSTR)m_pdb->RecPtr() + lblOffset);
				}
			}
			InitTextLabels(pDC, FALSE);
		}
	} //pDC

	STOP_NTI_TM(6);
	return (nRecsVisible==nRecsValid)?2:(nRecsVisible>0);
}

int CShpLayer::CopyToDIB(CDIBWrapper *pDestDIB, const CFltRect &geoExt, double fScale)
{
	//pDestDIB				- Points to screen bitmap to which image data will be copied.
	//geoExt				- Geographical extent corresponding to bitmap.
	//fScale				- Screen pixels / geo-units.
	//
	//Returns:	0			- Some data visible but none copied to bitmap.
	//			1			- Only a portion of the visible data copied.
	//			2			- All visible data copied, possibly nothing.

	if (ShpType() == SHP_POLYGON)
		return CopyPolygonsToDIB(pDestDIB, geoExt, fScale);
	if (ShpType() == SHP_POLYLINE)
		return CopyPolylinesToDIB(pDestDIB, geoExt, fScale);

	ASSERT(ShpType() == SHP_POINT);

	bool bLabeling = (m_uFlags&((m_pDoc->m_uEnableFlags&NTL_FLG_LABELS) | NTL_FLG_SHOWLABELS)) != 0 &&
		LabelScaleRangeOK(CLayerSet::m_uScaleDenom);
	bool bMarking = (m_uFlags&((m_pDoc->m_uEnableFlags&NTL_FLG_MARKERS) | NTL_FLG_SHOWMARKERS)) != 0 &&
		m_mstyle.MarkerVisible();

	if (!bMarking && !bLabeling) return 2;

	if (bMarking && !(m_uFlags&NTL_FLG_NONSELECTABLE) && !CLayerSet::m_bPtNodeInitialized)
		m_pDoc->LayerSet().InitPtNode(pDestDIB, geoExt);

	ID2D1DCRenderTarget *pRT = NULL;
	CPlaceMark symbol;
	UINT nPoints = 0, nVisible = 0, rec = 0;

	if (bMarking) {
		for (it_fltpt p = m_fpt.begin(); p != m_fpt.end(); p++, rec++) {
			if ((m_pdbfile->vdbe[rec] & SHP_EDITDEL) || ++nPoints && !geoExt.IsPtInside(*p)) continue;
			nPoints++;
			if (!pRT) {
				pRT = pDestDIB->InitRT();
				if (!pRT || !symbol.Create(pRT, m_mstyle)) {
					pRT = NULL;
					ASSERT(0);
					break;
				}
				pRT->BeginDraw();
			}

			CPoint pt((int)(fScale*(p->x - geoExt.l)+0.5), (int)(fScale*(geoExt.t - p->y)+0.5));
			symbol.Plot(pRT, pt);

			if (m_pPtNode && !(m_uFlags&NTL_FLG_NONSELECTABLE))
				m_pPtNode->AddPtNode(pt.x, pt.y, this, rec + 1);
			nVisible++;
		}
	}

	if (pRT) {
		if (0 > pRT->EndDraw()) {
			ASSERT(0);
		}
	}

	if (bLabeling) {
		rec = nPoints = nVisible=0; //compute these again
		CDC *pDC = NULL;
		for (it_fltpt p = m_fpt.begin(); p != m_fpt.end(); p++, rec++) {
			if ((m_pdbfile->vdbe[rec] & SHP_EDITDEL) || !geoExt.IsPtInside(*p)) continue;
			nPoints++;
			if (!pDC) {
				if (!(pDC = pDestDIB->GetMemoryDC()) || !InitTextLabels(pDC, TRUE)) {
					ASSERT(0);
					pDC = NULL;
					break;
				}
				::SetTextAlign(pDC->GetSafeHdc(), TA_TOP | TA_LEFT| TA_NOUPDATECP); //TA_CENTER for polylines and polygons
			}
			CPoint pt((int)(fScale*(p->x - geoExt.l)+0.5), (int)(fScale*(geoExt.t - p->y)+0.5));
			CPoint textOff;
			textOff.x = textOff.y = max(m_mstyle.wSize / 2, 1);
			pt.Offset(textOff);
			if (rec + 1 > m_pdb->NumRecs()) {
				nVisible++;
				pDC->TextOut(pt.x, pt.y, new_point_str, LEN_NEW_POINT_STR);
			}
			else if (!m_pdb->Go(rec + 1)) {
				nVisible++;
				PrintLabel(pDC, pt, (LPCSTR)m_pdb->RecPtr() + lblOffset);
			}
			else {
				ASSERT(0);
			}
		}
		if(pDC) InitTextLabels(pDC, FALSE);
	}
	return (nPoints == nVisible) ? 2 : (nPoints>0);
}

static POINT ptPolygon[6]={{4,7},{8,7},{8,3},{13,3},{13,12},{4,12}};
static POINT ptPolyline1[4]={{7,3},{7,8},{4,8},{4,13}};
static POINT ptPolyline2[4]={{13,2},{13,7},{10,7},{10,12}};

void CShpLayer::UpdateImage(CImageList *pImageList)
{
	if(!pImageList) {
		ASSERT(FALSE);
		return;
	}

	if(!HBITMAP(m_bmImage)) {
		VERIFY(m_bmImage.LoadBitmap(IDB_WHITE));
	}

	CWindowDC dc(NULL);
	CDC dcMem;
	VERIFY(dcMem.CreateCompatibleDC(&dc));
	HBITMAP hBitmapOld;
	VERIFY(hBitmapOld=(HBITMAP)::SelectObject(dcMem.m_hDC,m_bmImage.GetSafeHandle()));
	HPEN hPen,hPenOld;
	HBRUSH hBrush,hBrushOld;
	VERIFY(hBrushOld=(HBRUSH)::SelectObject(dcMem.m_hDC,::GetStockObject(WHITE_BRUSH)));
	VERIFY(dcMem.PatBlt(0,0,20,18,PATCOPY));

	if(ShpType()!=SHP_POINT) {
		VERIFY(hPen=CreateStylePen(m_mstyle,TRUE));
		VERIFY(hPenOld=(HPEN)::SelectObject(dcMem.m_hDC,hPen));
		if(ShpType()==SHP_POLYGON) {
			VERIFY(hBrush=CreateStyleBrush(m_mstyle));
			VERIFY(::SelectObject(dcMem.m_hDC,hBrush));
			dcMem.Polygon(ptPolygon,6);
			VERIFY(DeleteObject(::SelectObject(dcMem.m_hDC,hBrushOld)));
		}
		else {
			dcMem.Polyline(ptPolyline1,4);
			dcMem.Polyline(ptPolyline2,4);
		}
		VERIFY(DeleteObject(::SelectObject(dcMem.m_hDC,hPenOld)));
	}
	else {
		if(m_uFlags&NTL_FLG_MARKERS) {
			float fLineWidth;
			WORD wSize=0;
			ID2D1DCRenderTarget *prt=NULL;
			if (m_mstyle.wSize>17) {
				wSize=m_mstyle.wSize;
				m_mstyle.wSize=17;
				if((fLineWidth=m_mstyle.fLineWidth)>3) m_mstyle.fLineWidth=3;
			}
			if (0 <= d2d_pFactory->CreateDCRenderTarget(&d2d_props,&prt) && 0 <= prt->BindDC(dcMem, CRect(0, 0, 20, 18))) {
				prt->BeginDraw();
				CPlaceMark::PlotStyleSymbol(prt, m_mstyle, CPoint(10, 8));
				VERIFY(0 <= prt->EndDraw());
			}
#ifdef _DEBUG
			else ASSERT(0);
#endif
			SafeRelease(&prt);
			if(wSize) {
				m_mstyle.wSize=wSize;
				m_mstyle.fLineWidth=fLineWidth;
			}
		}
	}
	VERIFY(::SelectObject(dcMem.m_hDC,hBrushOld)); //dcMem still OK?
	::SelectObject(dcMem.m_hDC,hBitmapOld);

#ifdef _DEBUG
	int iCount=pImageList->GetImageCount();
#endif

	if(m_nImage==2) {
		m_nImage=pImageList->Add(&m_bmImage,(CBitmap *)NULL);
		ASSERT(m_nImage>2);
	}
	else {
		ASSERT(m_nImage>2);
		VERIFY(pImageList->Replace(m_nImage,&m_bmImage,(CBitmap *)NULL));
	}
#ifdef _DEBUG
	iCount=pImageList->GetImageCount();
	iCount=0;
#endif
}

