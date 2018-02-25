#include "stdafx.h"           
#include "walls.h"
#include "prjdoc.h"
#include "ntfile.h"
#include <trxfile.h>
#include "compview.h"
#include "plotview.h"
#include "segview.h"
#include "compile.h"
#include "wall_shp.h"

BOOL LoadNTW()
{
	DWORD i,size;
	NTW_HEADER ntw_hdr;
	POINT *pPts=NULL;
	int fh=-1;


	char *ntwpath=CPrjDoc::m_pReviewDoc->WorkPath(CPrjDoc::m_pReviewNode,CPrjDoc::TYP_NTW);

	if(_stat_fix(ntwpath,&size) || -1==(fh=_open(ntwpath,_O_BINARY|_O_SEQUENTIAL|_O_RDONLY))) {
		#ifdef _DEBUG
		   CMsgBox(MB_ICONEXCLAMATION,"Failed to open %s",ntwpath);
		#endif
		goto _errOpen;
	}

	if(_read(fh,&ntw_hdr,sizeof(ntw_hdr))!=sizeof(ntw_hdr)) {
#ifdef _DEBUG
		CMsgBox(MB_ICONEXCLAMATION, "Failure reading %u-byte NTW header.", sizeof(ntw_hdr));
#endif
		goto _errRead;
	}
	if(size!=(DWORD)(sizeof(ntw_hdr)+
		ntw_hdr.nPoints*sizeof(POINT)+
		ntw_hdr.nPolygons*sizeof(NTW_POLYGON)+
		ntw_hdr.nPolyRecs*sizeof(NTW_RECORD)+
		ntw_hdr.nMaskName*sizeof(SHP_MASKNAME))) {
#ifdef _DEBUG
		  CMsgBox(MB_ICONEXCLAMATION, "Bad NTW format: %s", ntwpath);
#endif
		  goto _errFormat;
		}

	if(!(pPts=(POINT *)malloc(i=size-sizeof(ntw_hdr)))) {
#ifdef _DEBUG
		CMsgBox(MB_ICONEXCLAMATION, "Failure allocating %u bytes for NTW point data.", i);
#endif
		goto _errMalloc;
	}

	if(i!=_read(fh,pPts,i)) {
#ifdef _DEBUG
		CMsgBox(MB_ICONEXCLAMATION, "Failure reading %u bytes of NTW point data.", i);
#endif
		goto _errRead;
	}
	_close(fh);

	CCompView::m_NTWhdr=ntw_hdr;
	CCompView::m_pNTWpnt=pPts;
	CCompView::m_pNTWply=(NTW_POLYGON *)(pPts+ntw_hdr.nPoints);
	return TRUE;

_errRead:
_errMalloc:
_errFormat:
	if(fh!=-1) _close(fh);
	free(pPts);

_errOpen:
	return FALSE;
}

static HPEN GetSVGPen()
{
	styletyp *styleOL=pSV->OutlineStyle();
	int penIdx=styleOL->LineIdx();
	if(penIdx==CSegView::PS_NULL_IDX) return NULL;
	int	nWidth=1;
	if(penIdx==CSegView::PS_HEAVY_IDX) {
		//approximate wall width thickness -- try 1 point.
		//The transform will scale this number to 1 pt thickness at 1 in == 30 ft map scale.
		//Hence, nWidth*scale=assumed width of pen in world meters. At a printed scale of
		//1:360 or 1 in = 30 ft, a 1 pt pen width corresponds to 360 pts = 5 in = 0.127 m.

		nWidth=(int)((CSegView::m_fThickPenPts*0.0635)/CCompView::m_NTWhdr.scale);
	}
 	return ::CreatePen(PS_SOLID,nWidth,pMF->bUseColors?styleOL->LineColor():RGB_BLACK);
}

BOOL PlotSVGMaskLayer()
{
	if(!CCompView::m_pNTWpnt && !LoadNTW()) return FALSE;

	AfxGetApp()->BeginWaitCursor();

	XFORM xf;
	HPEN hPen=GetSVGPen();
	getXform(&xf);

	int oldDC;
	VERIFY(oldDC=pvDC->SaveDC());
	VERIFY(::SetGraphicsMode(pvDC->m_hDC,GM_ADVANCED));
	VERIFY(::SetWorldTransform(pvDC->m_hDC,&xf));

	if(!hPen) hPen=(HPEN)::GetStockObject(NULL_PEN);

	::SelectObject(pvDC->m_hDC,hPen);

	POINT *pPnt=CCompView::m_pNTWpnt;
	NTW_POLYGON *pnPly=CCompView::m_pNTWply;
	int nPly=CCompView::m_NTWhdr.nPolygons;
	int nPts;
	HBRUSH h,hLastBrush,hNullBrush,hAltBrush,hFloorBrush,hBackBrush;
	COLORREF altClr=(COLORREF)-1;

	hAltBrush=NULL;
	hBackBrush=pSV->m_hBrushBkg;
	VERIFY(hFloorBrush=::CreateSolidBrush(pMF->bUseColors?pSV->FloorColor():RGB_WHITE));
	VERIFY(hNullBrush=(HBRUSH)::GetStockObject(NULL_BRUSH));

	VERIFY(::SelectObject(pvDC->m_hDC,hLastBrush=hFloorBrush));

	//Draw remaining polygons with background fill color --
	for(int i=0;i<nPly;i++) {
		nPts=(pnPly->flgcnt&NTW_FLG_MASK);
		if(pnPly->flgcnt&NTW_FLG_OPEN) {
			VERIFY(pvDC->Polyline(pPnt,nPts));
		}
		else {
			if(pnPly->flgcnt&NTW_FLG_NOFILL) h=hNullBrush;
			else if(pnPly->flgcnt&NTW_FLG_FLOOR) h=hFloorBrush;
			else if(pnPly->flgcnt&NTW_FLG_BACKGROUND) h=hBackBrush;
			else {
				if(altClr!=pnPly->clr) {
					if(hAltBrush) ::DeleteObject(hAltBrush);
					VERIFY(hAltBrush=::CreateSolidBrush(altClr=pnPly->clr));
				}
				h=hAltBrush;
			}
			if(h!=hLastBrush) ::SelectObject(pvDC->m_hDC,hLastBrush=h);
			//if(h==hAltBrush) ::SelectObject(pvDC->m_hDC,::GetStockObject(NULL_PEN));
 			VERIFY(pvDC->Polygon(pPnt,nPts));
			//if(h==hAltBrush) ::SelectObject(pvDC->m_hDC,hPen);
		}
		pPnt+=nPts;
		pnPly++;
	}

	VERIFY(::ModifyWorldTransform(pvDC->m_hDC,NULL,MWT_IDENTITY));
	VERIFY(pvDC->RestoreDC(oldDC));
	if(hAltBrush) ::DeleteObject(hAltBrush);
	if(hFloorBrush) ::DeleteObject(hFloorBrush);
	if(hPen) ::DeleteObject(hPen);

	AfxGetApp()->EndWaitCursor(); 
	return TRUE;
}
