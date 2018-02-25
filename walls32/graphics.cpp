// graphics.cpp : Graphics opperations for CPrjDoc
//
#include "stdafx.h"           
#include "walls.h"
#include "prjdoc.h"
#include "ntfile.h"
#include <trxfile.h>
#include "compview.h"
#include "plotview.h"
#include "segview.h"
#include "mapframe.h"
#include "mapview.h"
#include "pageview.h"
#include "linkedit.h"
#include "viewdlg.h"
#include "vnode.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#include "compile.h"

#define MIN_SCALE_WIDTH 15.0

//Used only for frame plotting --
static HPEN mapPen;
static COLORREF mapPenColor;
static int mapPenIdx;
static BYTE FAR *mapStr_flag;
static UINT nFloatCnt;
static WORD mapSeg;
static CSize mapSize;
static BOOL mapPenVisible,mapUseColors;
static SIZE szMarker;

//Minimum pixels per meter to enable Identify Vector mode --
#define VNODE_MIN_SCALE 0.5
static CVNode *pVnode,*pVnodeF;

#define pFlagStyle CCompView::m_pFlagStyle

static int xmin,xmax,ymin,ymax;
static int iSizeFixBox,iOffLabelX,iOffLabelY;
static POINT ptEnd;
static BOOL bChkRange,bDefault,bPlotPrefixes,bPlotHeights;
static BOOL bInitialView;
static int iDefView;
static HBRUSH hbrBoxDrawn;
static int iStrFlag;
static CRect rectLastTrav;
static int spcx,spcy;
static int iLabelSpace,mapWidth,mapDiv;
static UINT uLabelCnt,uLabelInc;
static WORD *pLabelMap;


#ifdef _DEBUG
WORD  *hlimit;
#endif

#define MAX_FRAMES 50
typedef struct {
	double xoff,yoff;
	double mapMeters;
} frametyp;
static frametyp frame[MAX_FRAMES];
static int nFRM;

static double sinA,cosA;
static double midE,midN,midU;

//Poly drawing variables used only by PlotFrame() --
#define MAX_PATH_LEN	1000
static int iPolyCnt;
static BOOL bPolyMove,bPolyActive;

static UINT last_end_pfx;

static void GetPixelSize(SIZE *ps,int size)
{
	//Assume size is in points and convert ps to logical units --
	if(CPlotView::m_uMetaRes) {
		ps->cx=ps->cy=(size*CPlotView::m_uMetaRes+36)/72;
	}
	else {
		ps->cx=(size*pvDC->GetDeviceCaps(LOGPIXELSX)+36)/72;
		ps->cy=(size*pvDC->GetDeviceCaps(LOGPIXELSY)+36)/72;
	}
}

double TransAx(double *pENU)
{
	return (*pENU-midE)*cosA - (pENU[1]-midN)*sinA;
}

double TransAy(double *pENU)
{
	return bProfile?(pENU[2]-midU):((*pENU-midE)*sinA + (pENU[1]-midN)*cosA);
}

//Accessed only in plotlrud.cpp --
double TransAz(double *pENU)
{
	ASSERT(bProfile);
	double view=(fView+90.0)*U_DEGREES;
	return (*pENU-midE)*cos(view) - (pENU[1]-midN)*sin(view);
}

PXFORM getXform(PXFORM pf)
{
	double sA=sinA;
	double cA=-cosA;

	double sx=xscale*CCompView::m_NTWhdr.scale;
	double sy=yscale*CCompView::m_NTWhdr.scale;
	double mx=-CCompView::m_NTWhdr.datumE+midE;
	double my=CCompView::m_NTWhdr.datumN-midN;
	pf->eM11=-(FLOAT)(sx*cA);
	pf->eM21=(FLOAT)(-sx*sA);
	pf->eM12=-(FLOAT)(sy*sA);
	pf->eM22=(FLOAT)(sy*cA);
	pf->eDx=(FLOAT)(xscale*(cA*mx-sA*my-xoff));
	pf->eDy=(FLOAT)(yscale*(sA*mx+cA*my-yoff));
	return pf;
}

static apfcn_v NewPltSeg(int iPlt,UINT pltrec)
{
    if(iPlt>=nSTR) {
      //What causes this?
      ASSERT(FALSE);
      goto _err;
    }
	pltseg *ps;
	if(nPLTSEGS==nLen_piPLTSEG) {
	  ps=(pltseg *)realloc(piPLTSEG,(nPLTSEGS+PLTSEG_INCREMENT)*sizeof(pltseg));
	  if(!ps) {
    	CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_WORKSPACE);
	    goto _err;
	  }
	  nLen_piPLTSEG+=PLTSEG_INCREMENT;
	  piPLTSEG=ps;
	}
	ps=piPLTSEG+nPLTSEGS;
	ps->rec=pltrec;
	ps->next=piPLT[iPlt];
	piPLT[iPlt]=++nPLTSEGS;
	return;
	
_err:
	nSTR=0;
}

static apfcn_v InvalidateRange(BOOL bTrav)
{
    CRect rect(xmin-1,ymin-1,xmax+1,ymax+1);
    if(hbrBoxDrawn) rect.InflateRect(iSizeFixBox,iSizeFixBox);
    if(bTrav) {
      if(bChkRange) {
        CRect rectUnion=rect;
        rect.UnionRect(&rectUnion,&rectLastTrav);
        rectLastTrav=rectUnion;
      }
      else rectLastTrav=rect;
    }
	if(bChkRange) {
      rect.OffsetRect(pPV->m_ptBmp);
	  pPV->InvalidateRect(&rect,FALSE);
	}
}

HPEN GetStylePen(styletyp *pStyle)
{
    COLORREF rgb=mapUseColors?pStyle->LineColor():RGB_BLACK;
    int idx=pStyle->LineIdx();
	if(idx==CSegView::PS_NULL_IDX) return NULL;
	return ::CreatePen(pSV->m_PenStyle[idx],(idx!=CSegView::PS_HEAVY_IDX)?1:pMF->iVectorThick,rgb);
}

static HPEN TickPenInit()
{
    //Tickmarks are black (same as frame outline) when printing.
    //regardless of the value of mapUseColors --
    
    COLORREF rgb = (mapUseColors && !pMF->bPrinter)?
                   pSV->m_pGridlines->Style()->LineColor():RGB_BLACK;
	return ::CreatePen(PS_SOLID,pMF->iTickThick,rgb);
}

CGradient *GetGradient(int idx)
{
	CGradient *grad=pSV->m_Gradient[idx];

	if(grad->StartValue()>=grad->EndValue() ||
		(idx==2 && CPrjDoc::m_iValueRangeNet!=(int)dbNTN.Position())) {
		double fMin,fMax;
		DWORD recsave=dbNTP.Position();

		CPrjDoc::GetValueRange(&fMin,&fMax,idx);
		CPrjDoc::InitGradValues(*grad,fMin,fMax,idx);

		VERIFY(!dbNTP.Go(recsave));
	}
	return grad;
}

COLORREF GetPltGradientColor(int idx)
{
	CGradient *pgrad=GetGradient(idx);

	if(idx!=1) {
		double dVal;
		if(!idx) dVal=(pPLT->xyz[2]+zLastPt)/2.0;
		else {
			if(!pPLT->str_id) dVal=GRAD_VAL_NONSTRING;
			else dVal=CPrjDoc::GetFRatio();
		}
		return pgrad->ColorFromValue(dVal);
	}

	return pgrad->ColorFromValue(trx_DateToJul(pPLT->date));
}

static apfcn_v InitMapPen(styleptr st)
{
   COLORREF rgb;
   
   if(mapUseColors) {
	   rgb=st->GradIdx()?clrGradient:st->LineColor();
   }
   else rgb=RGB_BLACK;

   int idx=st->LineIdx();
   HPEN hPen=NULL;
   int thickness=(idx==CSegView::PS_SOLID_IDX)?pMF->iVectorThin:
   			 ((idx==CSegView::PS_HEAVY_IDX)?pMF->iVectorThick:1);
   
   if(thickness>1 || idx>CSegView::PS_HEAVY_IDX || rgb)
      hPen=::CreatePen(pSV->m_PenStyle[idx],thickness,rgb);
   
   if(hPen) {
     mapPen=hPen;
     mapPenColor=rgb;
     mapPenIdx=idx;
   }
   else {
     mapPen=(HPEN)::GetStockObject(BLACK_PEN);
     mapPenColor=RGB_BLACK;
     mapPenIdx=0;
   }
   mapPenVisible=TRUE;
}

static apfcn_v PlotTicks()
{   
    int cx=mapSize.cx;
    int cy=mapSize.cy;
	int i;
	SIZE length;
	
	GetPixelSize(&length,pMF->iTickLength);
	
	//Use same color as grid unless printing--
	HPEN hPen=TickPenInit();
	HPEN hPenOld=(HPEN)::SelectObject(pvDC->m_hDC,hPen);
	
	//We must avoid accumulating roundoff due to limited dot resolution --
	double f;
	double fIntervalX=xscale*CSegView::m_pGridFormat->fTickInterval;
	double fIntervalY=yscale*CSegView::m_pGridFormat->fTickInterval;
	
	for(f=fIntervalX;;f+=fIntervalX) {
	    i=(int)(f+0.5);
	    if(i>=cx) break;
		pvDC->MoveTo(i,0);
		pvDC->LineTo(i,length.cy);
    }
	for(f=fIntervalX;;f+=fIntervalX) {
	    i=(int)(f+0.5);
	    if(i>=cx) break;
		pvDC->MoveTo(i,cy);
		pvDC->LineTo(i,cy-length.cy);
    }
	for(f=cy-fIntervalY;;f-=fIntervalY) {
	    i=(int)(f+0.5);
	    if(i<=0) break;
		pvDC->MoveTo(0,i);
		pvDC->LineTo(length.cx,i);
    }
	for(f=cy-fIntervalY;;f-=fIntervalY) {
	    i=(int)(f+0.5);
	    if(i<=0) break;
		pvDC->MoveTo(cx,i);
		pvDC->LineTo(cx-length.cx,i);
    }
	::DeleteObject(::SelectObject(pvDC->m_hDC,hPenOld));
}

enum e_outcode {O_LEFT=1,O_RIGHT=2,O_BOTTOM=4,O_TOP=8};
static int clipXmin,clipXmax,clipYmin,clipYmax;

static void SetClipRange(int overlapX,int overlapY)
{
	clipXmin=-overlapX;
	clipXmax=mapSize.cx+overlapX;
	clipYmin=-overlapY;
	clipYmax=mapSize.cy+overlapY;
}

UINT OutCode(double x,double y)
{
	UINT code=0;
	if(x<(double)clipXmin) code=O_LEFT;
	else if(x>(double)clipXmax) code=O_RIGHT;
	if(y<(double)clipYmin) code |= O_TOP;
	else if(y>(double)clipYmax) code |= O_BOTTOM;
	return code;
}

static apfcn_b ClipEndpoint(double x0,double y0,double *px1,double *py1,UINT code)
{
  //If necessary, adjusts *px1,*py1 to point on frame without changing slope.
  //Returns TRUE if successful (line intersects frame), otherwise FALSE.
  //Assumes code==OutCode(*px1,*py1) and that OutCode(x0,y0)&code==0;

  double slope;

  ASSERT((OutCode(x0,y0)&code)==0);

  if(code&(O_RIGHT|O_LEFT)) {
	slope=(*py1-y0)/(*px1-x0);
    *px1=(double)((code&O_RIGHT)?clipXmax:clipXmin);
    *py1=y0+slope*(*px1-x0);
	if(*py1<(double)clipYmin) code=O_TOP;
	else if(*py1>(double)clipYmax) code=O_BOTTOM;
	else code=0;
  }

  if(code&(O_TOP|O_BOTTOM)) {
	slope=(*px1-x0)/(*py1-y0);
    *py1=(double)((code&O_BOTTOM)?clipYmax:clipYmin);
    *px1=x0+slope*(*py1-y0);
	if(*px1<(double)clipXmin) code=O_LEFT;
	else if(*px1>(double)clipXmax) code=O_RIGHT;
	else code=0;
  }

  return code==0;
}

static apfcn_b ClipEndPoints(double &x0, double &y0, double &x1, double &y1)
{
	UINT out0=OutCode(x0,y0);
	UINT out1=OutCode(x1,y1);
	if((out0&out1) ||
	   (out1 && !ClipEndpoint(x0,y0,&x1,&y1,out1)) ||
	   (out0 && !ClipEndpoint(x1,y1,&x0,&y0,out0))) return FALSE;
	return TRUE;
}

static apfcn_v GetRange(double &minE,double &maxE,double &minN,double &maxN,int x,int y)
{
	double Tx=x/xscale+xoff;
	double Ty=-y/yscale-yoff;
	double d;
	
	d=Tx*cosA+Ty*sinA+midE;
	if(d<minE) minE=d;
	if(d>maxE) maxE=d;
	
	d=Ty*cosA-Tx*sinA+midN;
	if(d<minN) minN=d;
	if(d>maxN) maxN=d;
}

static apfcn_d GridX(double E,double N)
{
	
    return ((E-midE)*cosA-(N-midN)*sinA-xoff)*xscale;
}

static apfcn_d GridY(double E,double N)
{
    return (-((E-midE)*sinA+(N-midN)*cosA)-yoff)*yscale;
}

static apfcn_v PlotGrid()
{
	HPEN hPen=GetStylePen(pSV->m_pGridlines->OwnStyle());
	
	//Assume pen thickness is limited to CSegView::MAX_VECTORTHICK
	
	if(!hPen) return;
	
	HPEN hPenOld=(HPEN)::SelectObject(pvDC->m_hDC,hPen);
	
	double f;
	
    int cx=mapSize.cx;
    int cy=mapSize.cy;
    
	if(bProfile) {
		
		double fInterval=CSegView::m_pGridFormat->fVertInterval;
		int i=(int)(fInterval*yscale);
		
		if(i>CSegView::MAX_VECTORTHICK) {
		    //Draw elevation lines --
			//f=fmod(midU-yoff-(CSegView::m_pGridFormat->fVertOffset-fixoff[2]),fInterval);
			f=fmod(midU-yoff-CSegView::m_pGridFormat->fVertOffset,fInterval);
			while(TRUE) {
				i=(int)(f*yscale);
			    if(i>cy) break;
				if(i>0) {
					  pvDC->MoveTo(0,i);
					  pvDC->LineTo(cx,i);
				}
				f+=fInterval;
			}
		}
		
		fInterval*=CSegView::m_pGridFormat->fHorzVertRatio;
		i=(int)(fInterval*xscale);
		if(i>CSegView::MAX_VECTORTHICK) {
		    //Draw vertical lines --
			//f=fmod(sinA*(midN+fixoff[1])-cosA*(midE+fixoff[0])-
			f=fmod(sinA*midN-cosA*midE-
			       xoff+CSegView::m_pGridFormat->fHorzOffset,fInterval);
			while(TRUE) {
				i=(int)(f*xscale);
			    if(i>cx) break;
				if(i>0) {
					  pvDC->MoveTo(i,0);
					  pvDC->LineTo(i,cy);
				}
				f+=fInterval;
			}
		}
	}
	else {
		// For plans, we want to maintain invariant the survey's origin during a rotation:
		//
		//		  xoff' = xoff+TransAx(0,0)'-TransAx(0,0)
		//		        = xoff+midE*(cosA-cosA')-midN*(sinA-sinA')
		// 
		//		  yoff' = yoff-TransAy(0,0)'+TransAy(0,0)
		//		        = yoff-midE*(sinA-sinA')-midN*(cosA-cosA')
		//
		// So we will draw the grid as if the plan had been rotated
		// by -fGridNorth
		
		double eInterval=CSegView::m_pGridFormat->fEastInterval;
		double nInterval=eInterval*CSegView::m_pGridFormat->fNorthEastRatio;
		double minE=DBL_MAX,maxE=-DBL_MAX,minN=DBL_MAX,maxN=-DBL_MAX;

		midE-=CSegView::m_pGridFormat->fEastOffset; //-fixoff[0];
		midN-=CSegView::m_pGridFormat->fNorthOffset; //-fixoff[1];
		
		double xo,yo,sa,ca;
		BOOL bRotate=CSegView::m_pGridFormat->fGridNorth!=0.0;
		
		if(bRotate) {
		  xo=xoff; yo=yoff; sa=sinA; ca=cosA;
		  sinA=sin(cosA=U_DEGREES*(fView-CSegView::m_pGridFormat->fGridNorth));
		  cosA=cos(cosA);
		  xoff+=midE*(ca-cosA)-midN*(sa-sinA);
		  yoff-=midE*(sa-sinA)+midN*(ca-cosA);
		}
		
		GetRange(minE,maxE,minN,maxN,0,0);
		GetRange(minE,maxE,minN,maxN,0,cy);
		GetRange(minE,maxE,minN,maxN,cx,0);
		GetRange(minE,maxE,minN,maxN,cx,cy);
		
		double scalemin=xscale;
		if(scalemin>yscale) scalemin=yscale;

		//Draw North-South gridlines --
		if((int)(eInterval*scalemin)>CSegView::MAX_VECTORTHICK) {
			double x0,y0,x1,y1;
			f=minE-fmod(minE,eInterval);
			while(f<maxE) {
				x0=GridX(f,maxN); y0=GridY(f,maxN);
				x1=GridX(f,minN); y1=GridY(f,minN);
				if(ClipEndPoints(x0,y0,x1,y1)) {
					pvDC->MoveTo((int)x0,(int)y0);
					pvDC->LineTo((int)x1,(int)y1);
				}
				f+=eInterval;
			}
		}
		
		//Draw West-East gridlines --
		if((int)(nInterval*scalemin)>CSegView::MAX_VECTORTHICK) {
			double x0,y0,x1,y1;
			f=minN-fmod(minN,nInterval);
			while(f<maxN) {
				x0=GridX(minE,f); y0=GridY(minE,f);
				x1=GridX(maxE,f); y1=GridY(maxE,f);
				if(ClipEndPoints(x0,y0,x1,y1)) {
					pvDC->MoveTo((int)x0,(int)y0);
					pvDC->LineTo((int)x1,(int)y1);
				}
				f+=nInterval;
			}
		}
		
		midE+=CSegView::m_pGridFormat->fEastOffset; //-fixoff[0];
		midN+=CSegView::m_pGridFormat->fNorthOffset; //-fixoff[1];
		if(bRotate) {
		  xoff=xo; yoff=yo; sinA=sa; cosA=ca;
		}
	}
	::DeleteObject(::SelectObject(pvDC->m_hDC,hPenOld));
}

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

static apfcn_v PlotLabel()
{
	static char pfx_buf[SRV_SIZNAME+SRV_LEN_PREFIX_DISP+4];
	static UINT pfx_len;

	if(++uLabelCnt<uLabelInc) return;

	char *p=pPLT->name;
	int len=SRV_SIZNAME;

	if(bPlotPrefixes && pPLT->end_pfx) {
		if(pPLT->end_pfx!=last_end_pfx) {
			if(CSegView::GetPrefix(pfx_buf,pPLT->end_pfx,bPlotPrefixes)) {
				last_end_pfx=pPLT->end_pfx;
				pfx_len=strlen(p=pfx_buf);
				if(pfx_len) p[pfx_len++]=':';
				
			}
			else last_end_pfx=0;
		}
		else p=pfx_buf;

		if(p==pfx_buf) {
			memcpy(p+pfx_len,pPLT->name,SRV_SIZNAME);
			len+=pfx_len;
		}
	}

	while(len && p[len-1]==' ') len--;
	int x=ptEnd.x+iOffLabelX;
	int y=ptEnd.y+iOffLabelY;
	if(!iLabelSpace || LabelMap(x,y,p,len)) {
		if(CPlotView::m_uMetaRes) {
		  CSize size=pvDC->GetTextExtent(p,len);
		  if(x+size.cx>=mapSize.cx || y+size.cy>=mapSize.cy) return;
		}
		pvDC->TextOut(x,y,p,len);
		uLabelCnt=0;
	}
}

static apfcn_v PlotHeight()
{
	if(++uLabelCnt<uLabelInc) return;
	char p[30];
	int len=sprintf(p,"%-.1f",LEN_SCALE(pPLT->xyz[2]));
	int x=ptEnd.x+iOffLabelX;
	int y=ptEnd.y+iOffLabelY;
	if(!iLabelSpace || LabelMap(x,y,p,len)) {
		if(CPlotView::m_uMetaRes) {
		  CSize size=pvDC->GetTextExtent(p,len);
		  if(x+size.cx>=mapSize.cx || y+size.cy>=mapSize.cy) return;
		}
		pvDC->TextOut(x,y,p,len);
		uLabelCnt=0;
	}
}

static apfcn_v PlotFixBox()
{
	hbrBoxDrawn=(HBRUSH)::SelectObject(pvDC->m_hDC,::GetStockObject(NULL_BRUSH));
	pvDC->Rectangle(ptEnd.x-iSizeFixBox,ptEnd.y-iSizeFixBox,
	  ptEnd.x+iSizeFixBox+1,ptEnd.y+iSizeFixBox+1);
	::SelectObject(pvDC->m_hDC,hbrBoxDrawn);
}

static apfcn_v PolyInit()
{
	iPolyCnt=0;
	bPolyMove=FALSE; //not necessary?
	bPolyActive=CPlotView::m_pathPoint ||
		(CPlotView::m_pathPoint=(POINT *)malloc(MAX_PATH_LEN*sizeof(POINT)));
}

static apfcn_v PolyFlush()
{
	//***??? ASSERT(iPolyCnt>1);
	if(iPolyCnt>1) pvDC->Polyline(CPlotView::m_pathPoint,iPolyCnt);
	iPolyCnt=0;
}

static apfcn_v PlotEndpoint(int x,int y,int typ)
{
	if(bPolyActive) {
		if(!typ) {
			if(iPolyCnt) PolyFlush();
			bPolyMove=TRUE;
		}
	    else {
		  if(iPolyCnt>=MAX_PATH_LEN) {
			  PolyFlush();
			  bPolyMove=TRUE;
		  }
		  POINT *pt=&CPlotView::m_pathPoint[iPolyCnt];
		  if(bPolyMove) {
			 bPolyMove=FALSE;
			 pt->x=ptEnd.x;
			 pt->y=ptEnd.y;
			 pt++;
			 iPolyCnt++;
		  }
		  pt->x=x;
		  pt->y=y;
		  iPolyCnt++;
	  }
	  ptEnd.x=x; ptEnd.y=y;
	  return;
	}

	if(typ) pvDC->LineTo(x,y);
	else pvDC->MoveTo(x,y);
	ptEnd.x=x; ptEnd.y=y;
	//bChkRange TRUE only in PlotTraverse(), not PlotFrame() --
	if(bChkRange) {
		if(x<xmin) xmin=x;
		if(x>xmax) xmax=x;
		if(y<ymin) ymin=y;
		if(y>ymax) ymax=y;
	}
}

apfcn_v PlotVector(int typ)
{
	static double x0,y0;
	UINT bClip;
	double x,y,xsav,ysav;

	//typ==0 - MoveTo
	//typ==1 - DrawTo

	//Globals used for scaling --
	//  xoff,yoff         - Offset in survey units of upper left of bitmap
	//  xscale,yscale     - Logical units (pixels) when multiplied by survey units

	if(!pPLT->end_id) {
		//If this is a Draw to the zero datum and the last point was plotted,
		//draw a box around it. For PlotFrame() iSizeFixBox==0.                            
        if(iSizeFixBox && typ && !bClipped) PlotFixBox();
		bLastRef=TRUE;
		bClipped=FALSE;
		return;
	}

	x=xsav=(TransAx(pPLT->xyz)-xoff)*xscale;
	y=ysav=(-TransAy(pPLT->xyz)-yoff)*yscale;

	bClip=OutCode(x,y);

	//The vector is entirely outside if (but not only if) bClip&bClipped != 0.
	if(bClip&bClipped) goto _exitplot;

	if(!typ || bLastRef) {
		//move to --
		if(!bClip) {
		  PlotEndpoint((int)x,(int)y,0);
          /****if(bLastRef && iSizeFixBox) PlotFixBox();*/
          if(typ && iSizeFixBox) PlotFixBox();
		}
	}
	else {
		//line to --
		//If necessary, move to point where vector enters frame --
		if(bClipped) {
			if(ClipEndpoint(x,y,&x0,&y0,bClipped)) {
				PlotEndpoint((int)x0,(int)y0,0);
			}
			else goto _exitplot;
		}
		ASSERT(OutCode(x0,y0)==0);
		if(!bClip || ClipEndpoint(x0,y0,&x,&y,bClip)) {
			if(pVnodeMap) {
				ASSERT(pPLT->vec_id);
				mapset(pVnodeMap,dbNTP.Position()-pNTN->plt_id1);
			}
			PlotEndpoint((int)x,(int)y,1);
		}
	}

_exitplot:
	bLastRef=FALSE;
	bClipped=bClip;
	x0=xsav;
	y0=ysav;
	zLastPt=pPLT->xyz[2]; //Used only when coloring by depth
}

static void InitLabelFont(int typ)
{
	static CFont cf;
	static CFont *pcfOld;

	if(typ) {
		COLORREF clr=mapUseColors?pSV->m_StyleHdr[typ].LineColor():RGB_BLACK;
		COLORREF backclr=(pMF->bPrinter?RGB_WHITE:pSV->BackColor());
		PRJFONT *pf=(typ==HDR_NOTES)?pMF->pfNoteFont:pMF->pfLabelFont;

		if(clr==backclr) {
			if(clr!=RGB_BLACK) clr=RGB_BLACK;
			else clr=RGB_WHITE;
		}
		pvDC->SetTextColor(clr);
		if(CPlotView::m_uMetaRes) {
			iOffLabelX=(pMF->iOffLabelX*CPlotView::m_uMetaRes+36)/72;
			iOffLabelY=(pMF->iOffLabelY*CPlotView::m_uMetaRes+36)/72;
		}
		else {
			iOffLabelX=(pMF->iOffLabelX*pvDC->GetDeviceCaps(LOGPIXELSX)+36)/72;
			iOffLabelY=(pMF->iOffLabelY*pvDC->GetDeviceCaps(LOGPIXELSY)+36)/72;
		}
		if(pMF->bPrinter) pcfOld=pf->ScalePrinterFont(pvDC,&cf);
		else pcfOld=pvDC->SelectObject(&pf->cf);
    }
    else {
		pvDC->SelectObject(pcfOld);
		if(pMF->bPrinter) cf.DeleteObject();
    }
}

static BOOL IsFlagged()
{
	NTA_NOTETREE_REC rec;
	CSegListNode *pNode;
	BYTE key[8],keybuf[8];

	if(!pFlagStyle) return FALSE;
	
	key[0]=5;
	key[1]=NTA_CHAR_FLAGPREFIX;
	*((long *)&key[2])=pPLT->end_id;

	ASSERT(NTA_NOTETREE==ixSEG.GetTree());

	if(!ixSEG.Find(key)) { 
		do {
			VERIFY(!ixSEG.Get(&rec,sizeof(rec),keybuf,8));
			ASSERT(M_ISFLAG(rec.seg_id));
			ASSERT(rec.id==(DWORD)pPLT->end_id);
			if(!M_ISFLAG(rec.seg_id)) break;
			if(!(pNamTyp[rec.id]&M_COMPONENT)) continue;
			if(!((pNode=pSV->SegNode(rec.seg_id&M_SEGMASK))->IsVisible())) continue;
			if(!(M_FLAGS&pNode->NoteFlags())) continue;
			if(!M_ISHIDDEN(pFlagStyle[*(WORD *)(keybuf+6)].seg_id)) return TRUE;
		}
		while(!ixSEG.FindNext(key));
	}

	return FALSE;
}

static BOOL FindFlagTreeRec(WORD id)
{
	NTA_NOTETREE_REC rec;
	CSegListNode *pNode;
	BYTE key[10];

	if(!pFlagStyle) return FALSE;
	
	key[0]=7;
	key[1]=NTA_CHAR_FLAGPREFIX;
	*((long *)&key[2])=pPLT->end_id;
	*((WORD *)&key[6])=id;
	if(ixSEG.Find(key)) return FALSE; //No flag of this name for this station

	if(ixSEG.GetRec(&rec,sizeof(rec))) return FALSE;
	ASSERT(M_ISFLAG(rec.seg_id));
	if(!(pNamTyp[rec.id]&M_COMPONENT)) return FALSE;
	if(!((pNode=pSV->SegNode(rec.seg_id&M_SEGMASK))->IsVisible())) return FALSE;
	if(!(M_FLAGS&pNode->NoteFlags())) return FALSE;

	return TRUE;
}

apfcn_b InitMarkerPen(FLAGSTYLE *pfs,COLORREF clr,WORD style)
{
	//When called by PlotMarkers() --
	//pfs->clrBkgnd has pSV->BackColor() or white if printing,
	//clr is pSV->MarkerColor(), and style is pSV->MarkerStyle()

    if(M_ISHIDDEN(style)) return FALSE;

	COLORREF bckclr=pfs->clrBkgnd;
	BOOL bNewClr=FALSE;
	int isize=M_SYMBOLSIZE(style);

	if(isize==M_SIZEMASK) isize=pfs->dfltSize;
	if(!isize) return FALSE;

	clr=M_COLORRGB(clr);

	if(!pMF->bUseColors || clr==bckclr) {
		clr=RGB_BLACK;
		if(clr==bckclr) clr=RGB_WHITE;
	}

	if(!pfs->hPen || clr!=pfs->clrMarker) {
		HPEN hPen;
		LOGBRUSH lb;
		lb.lbColor=clr;
		lb.lbStyle=BS_SOLID;

		bNewClr=PS_INSIDEFRAME|PS_GEOMETRIC|PS_ENDCAP_FLAT|PS_JOIN_MITER;
		//Small circles look terrible otherwise --
		if(M_SYMBOLSHAPE(style)==FST_CIRCLES) bNewClr=PS_GEOMETRIC;

		hPen=(HPEN)::ExtCreatePen(bNewClr,pfs->iMarkerThick,&lb,0,NULL);
		if(!hPen) {
		  hPen=(HPEN)::GetStockObject(BLACK_PEN);
		  clr=RGB_BLACK;
		}
		if(pfs->hPen) ::DeleteObject(pfs->hPen);
		pfs->hPen=hPen;
		pfs->clrMarker=clr;
		bNewClr=TRUE;
	}

	if(M_SYMBOLSHAPE(style)==FST_TRIANGLES) isize++;

	GetPixelSize(&szMarker,isize);

	int ishade=M_SYMBOLSHADE(style);
	if(ishade>=FST_TRANSPARENT) ishade=FST_CLEAR;

	HBRUSH hBrush=pfs->hbrShade[ishade];

	switch(ishade) {
	  case FST_CLEAR:
		 if(!hBrush) hBrush=(HBRUSH)::GetStockObject(NULL_BRUSH);
		 break;
	  case FST_SOLID:
		 if(!hBrush || bNewClr || (WORD)ishade!=pfs->shade) {
			 if(hBrush) ::DeleteObject(hBrush);
			 hBrush=::CreateSolidBrush(clr);
		 }
		 break;
	  case FST_OPAQUE:
		 if(!hBrush) hBrush=::CreateSolidBrush(bckclr);
		 break;
	}

	pfs->hbrShade[ishade]=hBrush;
	pfs->shade=(WORD)ishade;
	return TRUE;
}

static apfcn_i InitShape(UINT style,POINT *flagpoint)
{
	int isize;
	int ishape;
	
	if((ishape=M_SYMBOLSHAPE(style))>=M_NUMSHAPES) ishape=pMF->iFlagStyle;

	isize=szMarker.cx;

	if(!(isize=(isize+1)/2)) return -1;
	
    if(ishape==FST_TRIANGLES) {
	    flagpoint[0].x=isize;
	    flagpoint[1].x=-isize;
	    flagpoint[0].y=flagpoint[1].y=(int)(((long)(22*isize+11)*szMarker.cy)/(38*szMarker.cx));
	    flagpoint[2].y=-(int)(((long)(44*isize+22)*szMarker.cy)/(38*szMarker.cx));
	    flagpoint[2].x=0;
    }
    else {
   		flagpoint[0].x=-isize;
    	flagpoint[1].x=isize+1;
    	isize=(int)(((long)isize*szMarker.cy)/szMarker.cx);
    	flagpoint[0].y=-isize;
    	flagpoint[1].y=isize+1;
    }
    return ishape;
}
    
static apfcn_b ExcludeFlag(POINT *pt,int ishape)
{
	if(ishape==FST_TRIANGLES) {
		if(pt[0].x>=mapSize.cx || pt[1].x<=0 || pt[0].y>=mapSize.cy || pt[2].y<=0)
		  return TRUE;
	}
	else {
		if(pt[0].x<=0 || pt[1].x>=mapSize.cx || pt[0].y<=0 || pt[1].y>=mapSize.cy)
		  return TRUE;
	}
	return FALSE;
}

static apfcn_v PlotSymbol(FLAGSTYLE *pfs,WORD style)
{
	int i,ishape,ishade;
	POINT point[3];
	HPEN hPenOld;

	if((ishape=InitShape(style,point))==-1) return;

	VERIFY(hPenOld=(HPEN)::SelectObject(pvDC->m_hDC,pfs->hPen));

	if(ishape==FST_PLUSES) {
		//We do our own clipping due to the limitations of
		//metafiles and aberrant printer drivers --
		int i=(szMarker.cx+1)/2;
		int f=ptEnd.x-i;
		int t=ptEnd.x+i;
		if(pfs->iMarkerThick<=1) t++;
		if(f<clipXmin) f=clipXmin;
		if(t>clipXmax) t=clipXmax;
		if(f<t) {
			pvDC->MoveTo(f,ptEnd.y);
			pvDC->LineTo(t,ptEnd.y);
		}
		i=(szMarker.cy+1)/2;
		f=ptEnd.y-i;
		t=ptEnd.y+i;
		if(pfs->iMarkerThick<=1) t++;
		if(f<clipYmin) f=clipYmin;
		if(t>clipYmax) t=clipYmax;
		if(f<t) {
			pvDC->MoveTo(ptEnd.x,f);
			pvDC->LineTo(ptEnd.x,t);
		}
		pvDC->MoveTo(ptEnd.x,ptEnd.y);
		::SelectObject(pvDC->m_hDC,hPenOld);
		return;
	}

	if((ishade=M_SYMBOLSHADE(style))>=FST_TRANSPARENT) ishade=FST_CLEAR;

	for(i=0;i<3;i++) {
	  point[i].x+=ptEnd.x;
	  point[i].y+=ptEnd.y;
	}

	if(CPlotView::m_uMetaRes && ExcludeFlag(point,ishape)) return;

	HBRUSH hBrushOld=(HBRUSH)::SelectObject(pvDC->m_hDC,pfs->hbrShade[ishade]);
	
	switch(ishape) {
		case FST_SQUARES:
			pvDC->Rectangle(point[0].x,point[0].y,point[1].x,point[1].y);
			break;
		case FST_CIRCLES:
			pvDC->Ellipse(point[0].x,point[0].y,point[1].x,point[1].y);
			break;
		case FST_TRIANGLES:
			pvDC->Polygon(point,3);
			break;
		case FST_PLUSES:
			break;
	}
	if(ishade!=FST_CLEAR)
		pvDC->SetPixel(ptEnd.x,ptEnd.y,(ishade==FST_SOLID)?pfs->clrBkgnd:pfs->clrMarker);
	
	VERIFY(::SelectObject(pvDC->m_hDC,hBrushOld));
	VERIFY(::SelectObject(pvDC->m_hDC,hPenOld));
}

static void InitFS(FLAGSTYLE *pfs,BOOL bFlag,COLORREF bclr)
{
	memset(pfs,0,sizeof(FLAGSTYLE));
	pfs->clrBkgnd=pMF->bPrinter?RGB_WHITE:bclr;
	pfs->dfltSize=bFlag?pMF->iFlagSize:pMF->iMarkerSize;
	pfs->iMarkerThick=bFlag?pMF->iFlagThick:pMF->iMarkerThick;
}

static void DeleteFS(FLAGSTYLE *pfs)
{
	if(pfs->hPen) {
		::DeleteObject(pfs->hPen);
		if(pfs->hbrShade[FST_SOLID]) ::DeleteObject(pfs->hbrShade[FST_SOLID]); 
		if(pfs->hbrShade[FST_OPAQUE]) ::DeleteObject(pfs->hbrShade[FST_OPAQUE]);
		//No need to delete NULL brush for FST_CLEAR;
	}
}

void CPrjDoc::PlotFlagSymbol(CDC *pDC,CSize sz,BOOL bFlag,COLORREF fgclr,COLORREF bkclr,WORD style)
{
	FLAGSTYLE fs;
	pvDC=pDC;
	CPlotView::m_uMetaRes=FALSE;
	pMF=&pPV->m_mfFrame;
	ptEnd.x=sz.cx/2;
	ptEnd.y=sz.cy/2;
	clipXmin=clipYmin=0;
	clipXmax=sz.cx;
	clipYmax=sz.cy;
	InitFS(&fs,bFlag,bkclr);
	if(InitMarkerPen(&fs,fgclr,style)) PlotSymbol(&fs,style);
	DeleteFS(&fs);
}

static void AddFlagVnodes(int flagcnt)
{
	UINT irec;
	int nflags,end_id,x,y;
	double xyz[3];
	double *pxyz;

	pVnodeF=new CVNode;
	if(pVnodeF && !pVnodeF->Init(mapSize.cx,mapSize.cy,flagcnt)) {
		delete pVnodeF;
		pVnodeF=NULL;
		ASSERT(FALSE);
	}

	nflags=end_id=0;
	for(irec=pNTN->plt_id1;irec<=pNTN->plt_id2;irec++) {
		if(dbNTP.Go(irec)) {
			ASSERT(FALSE);
			return;
		}
		pxyz=pPLT->xyz;

		if(pPLT->vec_id) {
			if(pPLT->end_id) {
				if(!(pNamTyp[pPLT->end_id]&M_FLAGGED)) goto _next;

				if(pNamTyp[pPLT->end_id]&M_FIXED) {
					if(end_id) goto _next;
				}
				else if(pPLT->flag==NET_REVISIT_FLAG) goto _next;
				pxyz=pPLT->xyz;
			}
			else {
				//Otherwise the last station was a fixed point --
				ASSERT(end_id && (pNamTyp[end_id]&M_FIXED));
				if(!(pNamTyp[end_id]&M_FLAGGED)) goto _next;
				pxyz=xyz;
			}

			if(++nflags>flagcnt) {
				ASSERT(FALSE);
				break;
			}
			x=(int)((TransAx(pxyz)-xoff)*xscale);
			y=(int)((-TransAy(pxyz)-yoff)*yscale);
			VERIFY(pVnodeF->AddVnode(x,y,x,y,irec));
		}
_next:
		end_id=pPLT->end_id;
		memcpy(xyz,pPLT->xyz,sizeof(xyz));
	}
}

static int PlotFlags()
{
    UINT irec,nFlags=pixSEGHDR->numFlagNames;
	int flagcnt=0;
	FLAGSTYLE fs;

	//Clear flagged bit in pNamTyp --
	for(irec=dbNTV.MaxID();irec;irec--) pNamTyp[irec]&=~M_FLAGGED;
	if(!nFlags) return 0;

	InitFS(&fs,TRUE,pSV->BackColor());

	ixSEG.UseTree(NTA_NOTETREE);

	while(nFlags--) {
		int nFlagidx=pFlagStyle[nFlags].iseq;

		WORD style=pFlagStyle[nFlagidx].seg_id;
		if(!InitMarkerPen(&fs,pFlagStyle[nFlagidx].color,style)) continue;
 
		for(irec=pNTN->plt_id1;irec<=pNTN->plt_id2;irec++) {
			if(dbNTP.Go(irec)) break;
								  		
			PlotVector(0);
					
  			if((M_FLAGS&pNamTyp[pPLT->end_id])) {
				if(!bClipped && !bLastRef && pPLT->flag!=NET_REVISIT_FLAG) {
					if(FindFlagTreeRec(nFlagidx)) {
						if(pSV->IsNoOverlap()) {
							UINT n,idx;
							for(n=0;n<nFlags;n++) {
								idx=pFlagStyle[n].iseq;
								if(!M_ISHIDDEN(pFlagStyle[idx].seg_id) &&
									FindFlagTreeRec(idx)) break;
							}
							if(n<nFlags) continue;
						}
						if(!(pNamTyp[pPLT->end_id]&M_FLAGGED)) {
							flagcnt++;
							pNamTyp[pPLT->end_id] |= M_FLAGGED;
						}
						PlotSymbol(&fs,style);
					}
				}
  			}
		}
	}

	ixSEG.UseTree(NTA_SEGTREE);
	DeleteFS(&fs);
	return flagcnt;
}

static apfcn_v PlotRawTrav(FLAGSTYLE *pfs,plttyp *pPlt,int rec,int flags,BOOL bCheckSeg)
{
	#define _PTEST(m) ((flags&m) && (!bCheckSeg || (pNamTyp[pPLT->end_id]&M_VISIBLE)))

	plttyp *pNTPSav=pPLT; //save pointer to dbNTP record buffer
	pPLT=pPlt; //PlotVector() will use pPLT to obtain the coordinates
    DWORD *pVnodeMapSav=pVnodeMap;
	pVnodeMap=NULL;  //Disable flagging of plotted vectors

	int end_id=pPlt->end_id;
	int idx;
	BOOL bReverse=(pSTR->flag&NET_STR_REVERSE)!=0;
	WORD style=pSV->MarkerStyle();
	
	CRect rectFrame(0,0,mapSize.cx,mapSize.cy); //For checking if marker/label is needed
		
	//Position pen to string endpoint -- 
	PlotVector(0);

	//We need to mark/label the first point *only* if this is a reverse-attached
	//string (in which case we will not mark/label the last point) --	
	if(flags && bReverse && !bClipped && !bLastRef && rectFrame.PtInRect(ptEnd)) {
		if(_PTEST(M_MARKERS)) PlotSymbol(pfs,style);
		if(_PTEST(M_NAMES)) bPlotHeights?PlotHeight():PlotLabel();
	}
	
	while(rec) {
	    if(dbNTV.Go(rec)) {
	      ASSERT(FALSE);
	      break;
	    }
	    idx=pVEC->id[1]==end_id;
	    ASSERT(idx || end_id==pVEC->id[0]);
		if(idx) {
			pPlt->xyz[0]-=pVEC->xyz[0];
			pPlt->xyz[1]-=pVEC->xyz[1];
			pPlt->xyz[2]-=pVEC->xyz[2];
		}
		else {
			pPlt->xyz[0]+=pVEC->xyz[0];
			pPlt->xyz[1]+=pVEC->xyz[1];
			pPlt->xyz[2]+=pVEC->xyz[2];
		}
		
		pPlt->end_id=end_id=pVEC->id[1-idx];
		rec=pVEC->str_nxt_vc;
		
		if(!flags) {
		  if(!bCheckSeg || pSV->SegNode(pVEC->seg_id)->IsVisible()) PlotVector(1);
		  else PlotVector(0);
		}
		else if(!bReverse || rec) {
			PlotVector(0);
		  	if(!bClipped && !bLastRef && rectFrame.PtInRect(ptEnd)) {
		  	  if(_PTEST(M_MARKERS)) PlotSymbol(pfs,style);
		  	  if(_PTEST(M_NAMES)) {
		  	    memcpy(pPlt->name,pVEC->nam[1-idx],SRV_SIZNAME);
				pPlt->end_pfx=pVEC->pfx[1-idx];
		  	    bPlotHeights?PlotHeight():PlotLabel();
		  	  }
		  	}
		}
	}
	pVnodeMap=pVnodeMapSav;
	pPLT=pNTPSav;
}

apfcn_v ReverseEndpoint(double *pxyz)
{
	//Adjust pxyz[] so that the raw string vectors (starting with position
	//rec in dbNTV), when added to pxyz[], result in the adjusted string's
	//ending position.
	
	//pxyz[] is already set to the adjusted string's starting position.

    for(int i=0;i<3;i++) pxyz[i]+=pSTR->e_xyz[i]-pSTR->xyz[i];
}

static apfcn_i GetStrEndpoint(int rec,plttyp *pPlt)
{
	//Initializes *pPlt with a copy of the plot record containing the
	//starting coordinates of the string rec. Returns the starting dbNTV record no.

	ASSERT(rec>=nSTR_1 && rec<=nSTR_2);
	
	memset(pPlt,0,sizeof(plttyp));  //In case of unexpected error
	
    if(dbNTS.Go(rec)) return 0;
    
    int end_id=pSTR->id[0]; //The station id of the string's endpoint
    pltseg *ps;
    
	rec=piPLT[rec-pNTN->str_id1];
	
	for(;rec>0;rec=ps->next) {
		//Search next plot file segment for end_id --
		ps=piPLTSEG+rec-1;
		if(dbNTP.Go(ps->rec)) break;
		ASSERT(!pPLT->vec_id);
		//Scan contiguous range of NTP records --
		while(end_id!=pPLT->end_id && !dbNTP.Next() && pPLT->str_id);
		if(end_id==pPLT->end_id) break;
	}
	
	ASSERT(rec>0);
	
	if(rec<=0) return 0;
	
	memcpy(pPlt,pPLT,sizeof(plttyp));
	return pSTR->vec_id;
}

static apfcn_v PlotRawTraverse(int str_id,int flags,BOOL bCheckSeg)
{
	//Plot unadjusted version of traverse at pSTR (which is floated).
	//Observe segment visibility when bCheckSeg is TRUE.
	
	int vec_id;
	plttyp plt,pltsav;

	if(!(vec_id=GetStrEndpoint(str_id,&plt))) return;

	if((pSTR->flag&NET_STR_REVERSE)!=0) ReverseEndpoint(plt.xyz); 
	
	//plt.xyz now has the map coordinates of the string endpoint --
	
	InitMapPen(pSV->RawTravStyle());
	
	if(mapPen) {
	    //Vectors are being drawn --
	    //We may need to save the name and starting coordinates --
	    if(flags) memcpy(&pltsav,&plt,sizeof(plttyp));
	    HPEN hPenOld=(HPEN)::SelectObject(pvDC->m_hDC,mapPen);
		PlotRawTrav(NULL,&plt,vec_id,0,bCheckSeg);  //deletes mapPen
		VERIFY(mapPen==(HPEN)::SelectObject(pvDC->m_hDC,hPenOld));
		::DeleteObject(mapPen);
	    if(flags) memcpy(&plt,&pltsav,sizeof(plttyp));
	}
	
	if(flags&=(M_NAMES+M_MARKERS)) {
		if(flags&M_NAMES) InitLabelFont(HDR_LABELS);
		FLAGSTYLE fs;
		InitFS(&fs,FALSE,pSV->BackColor());
	    if(InitMarkerPen(&fs,pSV->MarkerColor(),pSV->MarkerStyle()))
		  PlotRawTrav(&fs,&plt,vec_id,flags,bCheckSeg);
		DeleteFS(&fs);
		if(flags&M_NAMES) InitLabelFont(0);
	}
}

static apfcn_v PlotRawString(int idx,BOOL bFrag)
{
	//Traverse the string vectors, drawing the unadjusted lines.
	//idx is the string's record number in dbNTS --
	
	plttyp plt;

	if(!(idx=GetStrEndpoint(idx,&plt))) return;
	
	int end_id=plt.end_id; 
	
	if(iStrFlag&NET_STR_REVERSE) ReverseEndpoint(plt.xyz);
	
	CPen *penOld=pvDC->SelectObject(bFrag?&CReView::m_PenFragRaw:&CReView::m_PenTravRaw);
	plttyp *pNTP=pPLT; //save pointer to dbNTP record buffer
	
	pPLT=&plt; //PlotVector() will use pPLT to obtain the coordinates
		
	//Position pen to string endpoint -- 
	PlotVector(0);
	
	//idx is the NTV record # of the first string vector.
	//end_id is the identifier of the appropriate vector endpoint.
	while(idx && !dbNTV.Go(idx)) {
	    idx=pVEC->id[1]==end_id;
	    ASSERT(idx || end_id==pVEC->id[0]);
		if(idx) {
			plt.xyz[0]-=pVEC->xyz[0];
			plt.xyz[1]-=pVEC->xyz[1];
			plt.xyz[2]-=pVEC->xyz[2];
		}
		else {
			plt.xyz[0]+=pVEC->xyz[0];
			plt.xyz[1]+=pVEC->xyz[1];
			plt.xyz[2]+=pVEC->xyz[2];
		}
		plt.end_id=end_id=pVEC->id[1-idx];
		PlotVector(1);
		idx=pVEC->str_nxt_vc;
	}
	
	pPLT=pNTP;
    pvDC->SelectObject(penOld);
}

apfcn_v PlotString(int recID,BOOL bRaw);

static apfcn_v PlotFragments(int recID)
{
    CPen *penOld=pvDC->SelectObject(&CReView::m_PenTravFrag);
	BOOL bPlotRawFrag=(pSTR_flag&(bProfile?NET_STR_CHNFLTXV:NET_STR_CHNFLTXH))!=0;
	int s=recID;
	while(TRUE) {
		VERIFY(s=piFRG[s]);
		s=(s<0)?piLNK[-s-1]:s-1;
		if(s==recID) break;
		PlotString(s,FALSE);
		if(bPlotRawFrag && !dbNTS.Go(piSTR[s]) && (pSTR_flag&(bProfile?NET_STR_CHNFLTV:NET_STR_CHNFLTH))) {
			//Plt raw version of unselected fragment --
			int iStrFlagSave=iStrFlag;
			iStrFlag=pSTR_flag;
			PlotRawString(piSTR[s],TRUE);
			iStrFlag=iStrFlagSave;
		}
	}
    pvDC->SelectObject(penOld);
}

static apfcn_v PlotString(int recID,BOOL bRaw)
{
  pltseg *ps;
  
  int rec=piSTR[recID];
  
  ASSERT(rec>=nSTR_1 && rec<=nSTR_2);

  if(bRaw) {
	 if(!dbNTS.Go(rec)) {
		iStrFlag=pSTR_flag;
		if(iStrFlag&(bProfile?NET_STR_FLTMSKV:NET_STR_FLTMSKH))	PlotRawString(rec,FALSE);
		if(piFRG[recID]) PlotFragments(recID);
	 }
  }
  
  rec=piPLT[rec-pNTN->str_id1];
  
  while(rec>0) {
    //Plot next segment --
    ps=piPLTSEG+rec-1;
    if(dbNTP.Go(ps->rec)) break;
    ASSERT(!pPLT->vec_id);
    PlotVector(0);
    
    //Plot contiguous range of NTP records --
    while(!dbNTP.Next() && pPLT->str_id) PlotVector(1);
    rec=ps->next;
  }
}
  
static apfcn_v PlotSystem(int rec)
{
  int strlim=piSYS[rec];
  if(rec) rec=piSYS[rec-1];
  for(;rec<strlim;rec++) PlotString(rec,FALSE);
}

static apfcn_v InitPenStyle(styleptr st)
{
	//Called only from LineVisible() --

	if(!st->IsNoLines()) {
		COLORREF clr=st->GradIdx()?(clrGradient=GetPltGradientColor(st->GradIdx()-1)):st->LineColor();

		if(mapUseColors && (clr!=mapPenColor) ||
		   (st->LineIdx()!=mapPenIdx))
		{
			if(bPolyActive && iPolyCnt) {
				int i=pPLT->vec_id?iPolyCnt:0;
				PolyFlush();
				if(i) {
				  if(i>1) CPlotView::m_pathPoint[0]=CPlotView::m_pathPoint[i-1];
				  iPolyCnt++;
				}
			}
			InitMapPen(st);
			::DeleteObject(::SelectObject(pvDC->m_hDC,mapPen));
		}
		else mapPenVisible=TRUE;
	}
	else mapPenVisible=FALSE;
}

static BOOL LineVisible()
{
	if(mapStr_id && mapStr_id==pPLT->str_id) {
      if(mapSeg!=(WORD)-2) {
	      InitPenStyle(pSV->TravStyle());
	      mapSeg=(WORD)-2;
      }
    }
    else {
	    //seg_id is an index into an array of CSegListNode objects,
	    //which is initialized when workfiles are opened.
	    WORD seg_id=pPLT->seg_id;
	    
	    CSegListNode *pNode=pSV->SegNode(seg_id);
	    
	    if(pNode->IsVisible()) {
			ASSERT(!pPLT->str_id || pPLT->str_id-nSTR_1<nSTR);
			if(!mapStr_id && nFloatCnt && pPLT->str_id && mapStr_flag[pPLT->str_id-nSTR_1]) {
			  if(mapSeg!=(WORD)-2) {
				  InitPenStyle(pSV->TravStyle());
				  mapSeg=(WORD)-2;
			  }
			}
			else {
			  if(mapSeg!=seg_id || pNode->Style()->GradIdx()) {
				  InitPenStyle(pNode->Style());
		          mapSeg=seg_id;
	          }
	        }
	    }
	    else {
	      mapSeg=seg_id;
	      mapPenVisible=FALSE;
	    }
	}

	return (iPolysDrawn && maptst(pVnodeMap,dbNTP.Position()-pNTN->plt_id1))?FALSE:mapPenVisible;
}

int CPrjDoc::InitNamTyp()
{
    //Set an array of station flags to indicate whether marker/name visibility
    //is implied by that of the adjacent vectors. If any adjacent vector has
    //marker and/or name visibility then so does the station.
    //Also set flags to indicate adjacency of floated vectors.
    
    int irec;
   	UINT v,end_id;
  	CSegListNode *pNode;
  	
  	//ASSERT(!nSTR || pPV->m_iTrav>0);
  	
  	irec=(pPV->m_iTrav && pSV->IsTravVisible() && pSV->IsTravSelected())?
  		piSTR[pPV->m_iTrav-1]:0; 
    
    if(!pSV->IsNewVisibility() && mapStr_id==irec) return -1;
    mapStr_id=irec;
    
    ASSERT(dbNTV.Opened());
	UINT maxid=(int)dbNTV.MaxID()+1;  //Maximum possible value of a station ID plus one
		
    //In case older database is being opened --
    ASSERT(maxid<=(UINT)INT_MAX);
    
    nFloatCnt=0;

	//Avoid reallocating the array each time a frame is generated during this
	//review session --
	
	if(!pNamTyp) {
	  if((pNamTyp=(BYTE FAR *)calloc(maxid+nSTR+1,1))==NULL) {
	    mapStr_id=0;
	    return 0;
	  }
	  mapStr_flag=pNamTyp+maxid;
	}
	else {
		memset(pNamTyp, 0, maxid+nSTR+1);
	}
	
    UINT bFloatFlag=(nSTR && pSV->IsTravVisible())?
     (bProfile?NET_STR_FLTMSKV:NET_STR_FLTMSKH):0;
     
    if(bFloatFlag) {
		for(irec=nSTR_1;irec<=nSTR_2;irec++) {
	        if(!dbNTS.Go(irec) && (pSTR_flag&bFloatFlag)) {
	          mapStr_flag[irec-nSTR_1]|=1;
	          nFloatCnt++;
	        }
		}
	}
	
	BOOL bAllFloated=mapStr_id==0 && nFloatCnt!=0;
	
    end_id=0;
	for(DWORD prec=pNTN->plt_id1;prec<=pNTN->plt_id2;prec++,end_id=(UINT)pPLT->end_id) {
		ASSERT(end_id<maxid);
		if(dbNTP.Go(prec)) break;
		if(!pPLT->vec_id) continue;

		ASSERT(!pPLT->str_id || pPLT->str_id>=nSTR_1 && pPLT->str_id<=nSTR_2);

		v=M_COMPONENT;

		pNode=pSV->SegNode(pPLT->seg_id);
		
		if(pNode->IsVisible()) v|=M_VISIBLE;
		
		if(mapStr_id && mapStr_id==pPLT->str_id) {
		  v|=pSV->TravLabelFlags();
		}
		else if(bAllFloated && pPLT->str_id && mapStr_flag[pPLT->str_id-nSTR_1]) {
		  if(v&M_VISIBLE) v|=pSV->TravLabelFlags();
		}
		else if(v&M_VISIBLE) v|=pNode->LabelFlags();
		
		pNamTyp[end_id] |= v;

		if(!end_id) v|=M_FIXED; //for creating pVnodeF tree --

		if(pPLT->end_id) pNamTyp[pPLT->end_id] |= v;
		else {
			ASSERT(end_id);
			pNamTyp[end_id]|=M_FIXED;
		}
	}
  	pNamTyp[0]=0; //<REF> is never visible
  	
	pSV->SetNewVisibility(FALSE);
	return 1;
}

static apfcn_v InitNoteFlags(void)
{
	NTA_NOTETREE_REC rec;
	CSegListNode *pNode;
	WORD i;
	
	ASSERT(ixSEG.Opened());
	ixSEG.UseTree(NTA_NOTETREE);
	
	if(ixSEG.NumRecs()) {
	   if(pixSEGHDR->numFlagNames>0 && !pFlagStyle) {
	     pFlagStyle=(NTA_FLAGSTYLE *)calloc(pixSEGHDR->numFlagNames,sizeof(NTA_FLAGSTYLE));
	   }
	   if(!ixSEG.First()) {
	     do {
	       VERIFY(!ixSEG.GetRec(&rec,sizeof(rec)));
	       if(M_ISFLAGNAME(rec.seg_id)) {
			 i=(WORD)rec.id;
	         ASSERT(pFlagStyle && i<pixSEGHDR->numFlagNames);
			 if(pFlagStyle) {
				 pFlagStyle[i].color=rec.color;
				 pFlagStyle[i].seg_id=rec.seg_id;
				 i=*(WORD *)((BYTE *)&rec+2); /*iseq*/
				 ASSERT(i<pixSEGHDR->numFlagNames);
				 pFlagStyle[i].iseq=(WORD)rec.id;
			 }
	       }
	       else if((pNamTyp[rec.id]&M_COMPONENT) &&
	           (pNode=pSV->SegNode(rec.seg_id&M_SEGMASK))->IsVisible()) {
	         pNamTyp[rec.id]|=(rec.seg_id>>M_NOTESHIFT)&pNode->NoteFlags();
	       }
	     }
	     while(!ixSEG.Next());
	   }
	}
	
	ixSEG.UseTree(NTA_SEGTREE);
}

static apfcn_v PlotFrameOutline(CSize &size)
{
	VIEWFORMAT *pVF=&pPV->m_vfSaved[bProfile];
	int thick=pMF->iFrameThick;

	if(CPlotView::m_uMetaRes && thick>0) thick=1;

	if(pMF->iFrameThick) {
		CPen *pOldPen;
		CRect rect(0,0,size.cx,size.cy);
		rect.InflateRect(thick,thick);
		CPen pen(PS_INSIDEFRAME,thick,RGB_BLACK);
		pOldPen=pvDC->SelectObject(&pen);
		pvDC->Rectangle(&rect);
		pvDC->SelectObject(pOldPen);
	}

	if(VF_LEGPOS(pVF->bFlags)!=VF_LEGNONE) {
		char buf[256];
		TEXTMETRIC tm;
		CFont cf,*pfOld;

		pfOld=pMF->pfFrameFont->ScalePrinterFont(pvDC,&cf);
		
		char date[10],time[10];
		_strtime(time);
		time[5]=0;
		char view[6];
		strcpy(view,GetFloatStr(fView,1)); 
    
		int len=_snprintf(buf,256,"%s - ",CPrjDoc::m_pReviewNode->Title());

		//For multipage plots, indicate page number and frame ID --
		if(pGV->m_numsel>1 && !CPlotView::m_uMetaRes) {
			int npages=pGV->GetNumPages();
			if(npages>1) len+=_snprintf(buf+len,256-len,"Pg %u/%u, ",pPV->m_nCurPage,npages);
			len+=pGV->GetFrameID(buf+len,256-len);
		}

		len+=_snprintf(buf+len,256-len,"%s: %s\xB0  ",bProfile?"Profile":"Plan",view);
		  
		if(pSV->IsGridVisible()) {
			GRIDFORMAT *pGF=CSegView::m_pGridFormat;
			char *pUnits=LEN_ISFEET()?"ft":"m";
			if(CSegView::m_pGridFormat->wUseTickmarks) {
				len+=_snprintf(buf+len,256-len,"Ticks: %.0f %s  ",
				  LEN_SCALE(pGF->fTickInterval),pUnits);
			}
			else {
				double x,y;
				if(bProfile) {
				  x=pGF->fHorzVertRatio*pGF->fVertInterval;
				  y=pGF->fVertInterval;
				}
				else {
				  x=pGF->fEastInterval;
				  y=pGF->fNorthEastRatio*pGF->fEastInterval;
				}
				len+=_snprintf(buf+len,256-len,"Grid: %.0f x %.0f %s  ",
				  LEN_SCALE(x),LEN_SCALE(y),pUnits);
			}
		}
		
		len+=_snprintf(buf+len,256-len,"Scale: 1/%.0f  ",
			  (size.cx/xscale)*INCHES_PER_METER/pVF->fFrameWidth);

		len+=_snprintf(buf+len,256-len,"Printed: %s %s",_strdate(date),time);
		  
		pvDC->GetTextMetrics(&tm);

		{
			int x,y,pos=VF_LEGPOS(pVF->bFlags);
			BOOL bInside=CPlotView::m_uMetaRes || VF_ISINSIDE(pVF->bFlags);

			thick++;

			if(pos&VF_LEGTL) {
				//Top of frame
				y=bInside?(tm.tmExternalLeading+1):(-thick-tm.tmHeight-tm.tmExternalLeading-1);
			}
			else y=bInside?(size.cy-tm.tmHeight-2*tm.tmExternalLeading):(size.cy+thick+tm.tmExternalLeading);

			if(pos>=VF_LEGLR) {
				//Right side --
				SIZE sz;

				x=size.cx;
				VERIFY(::GetTextExtentExPoint(pvDC->m_hDC,buf,len,size.cx,&len,NULL,&sz));
				x-=sz.cx+(tm.tmAveCharWidth/2);
			}
			else x=bInside?(tm.tmAveCharWidth/2):(1-thick);
			pvDC->TextOut(x,y,buf,len);
		}

		pvDC->SelectObject(pfOld);
		cf.DeleteObject();
	}		
}

static void PlotMarkers(UINT flags)
{
    UINT flag,irec=pNTN->plt_id1;
	FLAGSTYLE fs;
	WORD style;
    
	if(dbNTP.Go(irec)) return;

	ixSEG.UseTree(NTA_NOTETREE);

	InitFS(&fs,FALSE,pSV->BackColor());
    if(InitMarkerPen(&fs,pSV->MarkerColor(),style=pSV->MarkerStyle())) {
    
		flags&=(M_MARKERS+M_FLAGS);
				
		for(;irec<=pNTN->plt_id2;irec++) {
								  		
			PlotVector(0);
					
  			if(!bClipped && !bLastRef && pPLT->flag!=NET_REVISIT_FLAG) {
				flag=(flags&pNamTyp[pPLT->end_id]);
				if(flag && (flag==M_MARKERS || !IsFlagged())) PlotSymbol(&fs,style);
  			}
			if(dbNTP.Next()) break;
		}
	}

	ixSEG.UseTree(NTA_SEGTREE);

	DeleteFS(&fs);
}

static void PlotLabels()
{
    UINT irec=pNTN->plt_id1;
    
	if(dbNTP.Go(irec)) return;
	
    InitLabelFont(HDR_LABELS);
			
	for(;irec<=pNTN->plt_id2;irec++) {
								  	
		PlotVector(0);
		
  	    if(!bClipped && !bLastRef && pPLT->flag!=NET_REVISIT_FLAG) {
  	       
  	         if(pNamTyp[pPLT->end_id]&M_NAMES) bPlotHeights?PlotHeight():PlotLabel();  	
  	    }
  	    
		if(dbNTP.Next()) break;
	}
			
	InitLabelFont(0);
}

BOOL CPrjDoc::FindNoteTreeRec(char *keybuf,BOOL bUseFirst)
{
	//Buffer size should be at least TRX_MAX_KEYLEN+1 = 255+1
	NTA_NOTETREE_REC rec;
	CSegListNode *pNode;
	BOOL bRet=FALSE;
	BYTE key[6];
	
	ixSEG.UseTree(NTA_NOTETREE);

	key[0]=5;
	key[1]=NTA_CHAR_NOTEPREFIX;
	*((long *)&key[2])=pPLT->end_id;
	if(ixSEG.Find(key)) goto _bRet;

	do {
	  if(ixSEG.GetRec(&rec,sizeof(rec))) break;
	  ASSERT(M_ISNOTE(rec.seg_id));
	  if(bUseFirst) goto _fillbuf;
	  if((pNamTyp[rec.id]&M_COMPONENT)) {
		  if((pNode=pSV->SegNode(rec.seg_id&M_SEGMASK))->IsVisible()) {
			  if((M_NOTES&pNode->NoteFlags())) {
				  goto _fillbuf;
			  }
		  }
	  }
	}
    while(!ixSEG.FindNext(key));

    ASSERT(FALSE);
	goto _bRet;

_fillbuf:
	VERIFY(!ixSEG.GetKey(keybuf));
	memmove(keybuf,_NOTESTR(keybuf),bRet=_NOTELEN(keybuf));
	keybuf[bRet]=0;
	bRet=TRUE;

_bRet:
	ixSEG.UseTree(NTA_SEGTREE);
	if(!bRet) *keybuf=0;
    return bRet;
}

static apfcn_v DrawMetaNote(int x,int y,char *p,int len)
{
	char *p1=p;
	SIZE size;
	int maxlen;

    while(len>0) {
        if(!(p1=strchr(p,'\n'))) p1=p+len;
		//Clipping apparently doesn't work in either EMF or WMF files!
        VERIFY(::GetTextExtentExPoint(pvDC->m_hDC,p,p1-p,mapSize.cx-x,&maxlen,NULL,&size));
		if(y+size.cy>=mapSize.cy) break;
		pvDC->TextOut(x,y,p,maxlen);
        y+=size.cy; //Same as pMF->pfNoteFont->LineHeight?
		p1++;
		len-=p1-p;
        p=p1;
    }
}

static apfcn_v PlotNote()
{
	char note[TRX_MAX_KEYLEN+1];
	
	if(!CPrjDoc::FindNoteTreeRec(note,FALSE)) return;
	
	int x=ptEnd.x+iOffLabelX;
	int y=ptEnd.y+iOffLabelY;
	int len=strlen(note);
    if(!iLabelSpace || LabelMap(x,y,note,-len)) {
		if(!CPlotView::m_uMetaRes) {
			CRect rect(x,y,mapSize.cx,mapSize.cy);
			pvDC->DrawText(note,len,&rect,0);
		
		}
		else DrawMetaNote(x,y,note,len);
	}
}

static void PlotNotes(UINT flags)
{
    UINT irec=pNTN->plt_id1;
    
	if(dbNTP.Go(irec)) return;

    InitLabelFont(HDR_NOTES);
    
	for(;irec<=pNTN->plt_id2;irec++) {
								  	
		PlotVector(0);
								  	  
  	    if(!bClipped && !bLastRef && (M_NOTES&pNamTyp[pPLT->end_id]) && 
  	       pPLT->flag!=NET_REVISIT_FLAG &&
		   (!(flags&M_FLAGS)||!pSV->IsHideNotes()||(M_FLAGGED&pNamTyp[pPLT->end_id]))) PlotNote();
		  	    
		if(dbNTP.Next()) break;
	}
	InitLabelFont(0);
}

static void FixClipRegion(int ovrX,int ovrY)
{
	RECT rect;
	rect.top=-ovrY;
	rect.left=-ovrX;
	rect.right=pPV->m_sizePrint.cx+ovrX;
	rect.bottom=pPV->m_sizePrint.cy+ovrY;
	::LPtoDP(pvDC->m_hDC,(LPPOINT)&rect,2);
	CRgn rgn;
	//Must use pixel units!
	rgn.CreateRectRgnIndirect(&rect);
	int i=pvDC->SelectClipRgn(&rgn,RGN_AND);
	ASSERT(i==SIMPLEREGION);
}

void CPrjDoc::InitTransform(CMapFrame *pf)
{
	pf->m_fView=fView;
	pf->m_scale=xscale;
	pf->m_xoff=xoff;
	pf->m_yoff=yoff;
	pf->m_midU=midU;
	pf->m_midE=midE;
	pf->m_midN=midN;
	pf->m_sinA=sinA;
	pf->m_cosA=cosA;
	
	if(pf->m_bProfile=bProfile) {
		//U=LEN_SCALE(midU-yoff)-LEN_SCALE(point.y-5)/xscale);
		pf->m_cosAx=LEN_SCALE(midU-yoff);
		pf->m_midNx=-LEN_SCALE(1.0/xscale);
	}
	else {
		pf->m_cosAx=LEN_SCALE(cosA/xscale);
		pf->m_sinAx=LEN_SCALE(sinA/xscale);
		pf->m_midEx=LEN_SCALE(xoff*cosA-yoff*sinA+midE);
		pf->m_midNx=LEN_SCALE(xoff*sinA+yoff*cosA-midN);
	}
}

static void InitVnode(DWORD numVecs)
{
	ASSERT(!pVnode && !CMapFrame::m_bKeepVnode && xscale>=VNODE_MIN_SCALE);
	
	pVnode=new CVNode;

	if(!pVnode || !pVnode->Init(mapSize.cx,mapSize.cy,numVecs)) {
		delete pVnode;
		pVnode=NULL;
		ASSERT(FALSE);
		return;
	}

	int x0,y0,x1,y1;
	int iBase=pNTN->plt_id1;
	int iLimit=pNTN->plt_id2-pNTN->plt_id1+1;
	int iNext=iLimit;

	for(int i=0;i<iLimit;i++) {
		if(maptst(pVnodeMap,i)) {
			if(i!=iNext) {
				VERIFY(!dbNTP.Go(iBase+i-1));
				ASSERT(pPLT->end_id);
				x0=(int)((TransAx(pPLT->xyz)-xoff)*xscale);
				y0=(int)((-TransAy(pPLT->xyz)-yoff)*yscale);
			}
			else {
				x0=x1; y0=y1;
			}
			VERIFY(!dbNTP.Next());
#ifdef _DEBUG
			ASSERT(pPLT->vec_id && pPLT->end_id);
#endif
			x1=(int)((TransAx(pPLT->xyz)-xoff)*xscale);
			y1=(int)((-TransAy(pPLT->xyz)-yoff)*yscale);
			VERIFY(pVnode->AddVnode(x0,y0,x1,y1,iBase+i));
			if(!--numVecs) break;
			iNext=i+1;
		}
	}
	ASSERT(!numVecs);
}

void CPrjDoc::PlotFrame(UINT flags)
{
	UINT irec,szpVnodeMap;
	CSize size;

	//Initialize global for static functions --

	if(!pSV->InitSegTree()) return;
  
	//Variables used when flags&M_PRINT==0
	CBitmap *pBmp=NULL;
	HBITMAP hBmpOld=NULL;
	HBRUSH  hBrushOld=NULL;
	CMapFrame *pFrame=NULL;
	HPEN    hPenOld=NULL;
		
	//Variables used for screen plotting --
	CDC dcBmp;
		
	ASSERT(dbNTN.Opened() && dbNTP.Opened());

	ASSERT((int)dbNTN.Position()==pPV->m_recNTN);

	if(dbNTP.Go(irec=pNTN->plt_id1)) {
	  //workfile corrupt --
	  ASSERT(FALSE);
	  return;
	}

	double sav_xscale=xscale;
	double sav_yscale=yscale;
	double sav_xoff=xoff;
	double sav_yoff=yoff;
	//To allow frame zooming --
	double sav_sinA=sinA;
	double sav_cosA=cosA;
	double sav_fView=fView;
	double sav_midU=midU;
	double sav_midE=midE;
	double sav_midN=midN;
	BOOL sav_bProfile=bProfile;
	bool bKeepScale=false;

	pVnode=pVnodeF=NULL;
	pVnodeMap=NULL;

	iPolysDrawn=0;
	bNoVectors=(flags&M_NOVECTORS) || (!pSV->IsLinesVisible() && (!mapStr_id || pSV->TravStyle()->IsNoLines()));

	if(!(flags&M_METAFILE)) CPlotView::m_uMetaRes=0;

	if(flags&M_PRINT) { 
   
		size.cx=pPV->m_sizePrint.cx;
		size.cy=pPV->m_sizePrint.cy;
		xscale=(xscale*size.cx)/pPV->m_sizeBmp.cx; //more dots per meter
		yscale=xscale*pPV->m_fPrintAspectRatio;

		//Shift xoff,yoff to page currently being printed --
		pGV->AdjustPageOffsets(&xoff,&yoff,pPV->m_nCurPage);
		
		//Adjust yoff to leave center of frame invariant with respect to
		//frame's new height/width ratio --
		
		yoff+=((((double)pPV->m_sizeBmp.cy*size.cx)/pPV->m_sizeBmp.cx)-
		       size.cy/pPV->m_fPrintAspectRatio)/(2.0*xscale);
		
		pvDC=pPV->m_pDCPrint;

		if(CPlotView::m_uMetaRes) pMF=&CPlotView::m_mfExport;
		else pMF=&CPlotView::m_mfPrint;
	
		//First, draw a frame with annotation --
		PlotFrameOutline(size);

		//Establish final clipping region for remainder of output.
		//Note: The framework has already established a region
		//for the printer device --
		if(!CPlotView::m_uMetaRes) FixClipRegion(pPV->m_iOverlapX,pPV->m_iOverlapY);
	}
	else {
		//Display map in a separate screen window --
		//Get temporary DC for screen - Will be released in dc destructor
		CWindowDC dc(NULL);

		pFrame=m_pReviewDoc->m_pMapFrame;
		ASSERT(!pFrame || pFrame->m_pDoc==m_pReviewDoc);

		pMF=&pPV->m_mfFrame; //MAPFORMAT
		
		if(pPV->m_bExternUpdate>1) {
		   ASSERT(pFrame);
		   bProfile=pFrame->m_bProfile;
		   sinA=pFrame->m_sinA;
		   cosA=pFrame->m_cosA;
		   fView=pFrame->m_fView;
		   midE=pFrame->m_midE;
		   midN=pFrame->m_midN;
		   midU=pFrame->m_midU;
		   if(pPV->m_bExternUpdate==3) {
			   //CMapView() - called only after frame resizing by dragging border
			   bKeepScale=true;
			   xoff=pFrame->m_xoff; //same TL offsets
			   yoff=pFrame->m_yoff;
			   size=pFrame->m_pView->m_sizeClient;
			   xscale=pFrame->m_scale; //same scale
			   pBmp=new CBitmap;
			   //This allocates ? bytes of GPI private (assuming 1024x756x256) --
			   if(!pBmp->CreateCompatibleBitmap(&dc, size.cx, size.cy)) {
				   delete pBmp;
				   CMsgBox(MB_ICONEXCLAMATION, IDS_ERR_MAKEFRAME1, ::GetLastError());
				   goto _restore;
			   }
			   delete pFrame->m_pBmp;
			   pFrame->SetFrameSize(size); //view already established size, this allows disabling scrolling
			   pFrame->InitializeFrame(pBmp, size, xoff, yoff, xscale);
		   }
		   else {
			    ASSERT(pPV->m_bExternUpdate==2);
			    //Called from CMapView::RefreshView() (applying CSegView changes) and
				//CMapView::NewVew() (temporary adjustments, pan, zoom, center, etc.)
				//No window size change --
				xoff=pFrame->m_xoff+pFrame->m_pCRect->left/pFrame->m_scale;
				yoff=pFrame->m_yoff+pFrame->m_pCRect->top/pFrame->m_scale;
				size=pFrame->m_pView->m_sizeClient;
				double sc=(pFrame->m_scale*size.cx)/pFrame->m_pCRect->Width();
				if(sc==xscale) {
					bKeepScale=true;
				}
				else xscale=sc;
		   }
		}
		else {
			//Called from CPlotView using Display Map or Update View --
			ASSERT(!pPV->m_bExternUpdate || pPV->m_bExternUpdate==1);
			double pvxscale(xscale);

			int pvwidth=pPV->m_sizeBmp.cx;
			int pvheight=pPV->m_sizeBmp.cy;
			double pvyratio=(double)pvheight/pvwidth;

			if(pPV->m_bTrackerActive) {
				//We are zooming into the tracker rectangle --
				CPoint posTracker;
				int trkw=pPV->GetTrackerPos(&posTracker); //width of tracker window in pixels
				xoff+=posTracker.x/xscale;
				yoff+=posTracker.y/yscale;
				pvxscale=(xscale*pvwidth)/trkw; //more dots per meter
			}

			if(pPV->m_bExternUpdate) {
				ASSERT(pPV->m_bExternUpdate==1);
				//Using the existing frame's client size (update from review dlg) --
				size=pFrame->m_pView->m_sizeClient;
				//Adjust xoff,yoff to place tracker or pPV bitmap center coordinate at client center --
				if(pvwidth*size.cy<pvheight*size.cx) {
					//wider plot -- decrease xoff
					//scale determined by size.cy (fit height)
					xscale=(pvxscale*size.cy)/pPV->m_sizeBmp.cy; //more dots per meter
					xoff-=(size.cx/xscale-pvwidth/pvxscale)/2;
				}
				else if(pvwidth*size.cy>pvheight*size.cx) {
					//taller plot -- decrease yoff
					//scale determined by size.cx (fit width)
					xscale=(pvxscale*size.cx)/pPV->m_sizeBmp.cx; //more dots per meter
					yoff-=(size.cy/xscale-(pvwidth*pvyratio)/pvxscale)/2;
				}
			}
			else {
			    //Creating a new CMapFrame/CMapView --
				//Use current defaulf frame width with aspect ratio same as CPlotView --
				size.cx=(int)(pMF->fFrameWidthInches*dc.GetDeviceCaps(LOGPIXELSX));
				size.cy=(int)(pvyratio*size.cx);
				xscale=(pvxscale*size.cx)/pvwidth; //more dots per meter
			}
		}

		yscale=xscale;

		if(!pPV->m_bExternUpdate) {

			pFrame=NULL;
				    
			pBmp=new CBitmap;
						
			//This allocates ? bytes of GPI private (assuming 1024x756x256) --
			if(!pBmp->CreateCompatibleBitmap(&dc,size.cx,size.cy)) {
				delete pBmp;
				CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_MAKEFRAME1,::GetLastError());
				goto _restore;
			}
		}
		else {
		   ASSERT(pFrame);
		   delete pFrame->m_pVnodeF;
		   pFrame->m_pVnodeF=NULL;
		   //ASSERT(!CMapFrame::m_bKeepVnode || (pFrame->m_pVnode && pPV->m_bExternUpdate==2)); 
		   if(!CMapFrame::m_bKeepVnode) {
				delete pFrame->m_pVnode;
				pFrame->m_pVnode=NULL;
		   }
		   VERIFY(pBmp=pFrame->m_pBmp);
		}

		if(xscale>=VNODE_MIN_SCALE && (!pFrame || !pFrame->m_pVnode)) {
			szpVnodeMap=(pNTN->plt_id2-pNTN->plt_id1+8*sizeof(DWORD))/(8*sizeof(DWORD));
			pVnodeMap=(DWORD *)calloc(szpVnodeMap,sizeof(DWORD));
		}
		
	    // Create compatible memory DC (global CDC pointer is used by PlotVector) --
	    
	    VERIFY(dcBmp.CreateCompatibleDC(&dc));
		    
	    //CBitmap *pBmpOld;
	    //VERIFY(pBmpOld = dcBmp.SelectObject(&m_cBmp));
	    //VERIFY(hBmpOld = (HBITMAP) pBmpOld->GetSafeHandle());
		    
	    hBmpOld=(HBITMAP)::SelectObject(dcBmp.m_hDC,pBmp->GetSafeHandle());
	    if(pMF->bUseColors) hBrushOld=(HBRUSH)::SelectObject(dcBmp.m_hDC,pSV->m_hBrushBkg);
	    
		dcBmp.PatBlt(0,0,size.cx,size.cy,PATCOPY); //Fill bitmap with brush color
	    pvDC=&dcBmp;
    }
    
    //Code common to both printer and display --
    iLabelSpace=(flags&(M_NAMES+M_NOTES))?pMF->iLabelSpace:0;
	uLabelInc=pMF->iLabelInc;
	uLabelCnt=0;
    ASSERT(!pLabelMap);
    pLabelMap=NULL;
	
	bChkRange=bPlotPrefixes=bPlotHeights=FALSE;
	iSizeFixBox=0;
	
	pvDC->SetBkMode(TRANSPARENT); //Use throughout for lines and text background

    mapUseColors=pMF->bUseColors;
    mapSize=size; //Used by static routines 
	SetClipRange(0,0);
		
	if((flags&(M_PRINT+M_METAFILE))==M_PRINT) SetClipRange(pPV->m_iOverlapX,pPV->m_iOverlapY);
    
    if((flags&M_ALLFLAGS) || (nSTR && pSV->IsTravVisible())) {

		if(flags&M_NAMES) {
			if(!(flags&M_MAPFRAME) && pPV->m_bExternUpdate!=1) {
				if(pMF->bLblFlags&1) {
					flags|=M_HEIGHTS;
					ASSERT(!(flags&M_PREFIXES));
				}
				else if(pMF->bLblFlags&(2+4+8)) {
					flags|=M_PREFIXES;
					ASSERT(!(flags&M_HEIGHTS));
				}
			}
			bPlotHeights=(flags&M_HEIGHTS)!=0;
			bPlotPrefixes=(flags&M_PREFIXES)!=0;
			if(bPlotPrefixes) {
				bPlotPrefixes|=(pMF->bLblFlags&(8+4+2));
				last_end_pfx=0;
			}
		}
		//Set mapStr_id *or* nFloatCnt!=0 iff there exist floated vectors
		//that must be highlighted --
		if(irec=InitNamTyp()) {
			if((flags&M_NOTEFLAGS) && (int)irec>0) InitNoteFlags();
			if(iLabelSpace) InitLabelMap();
		}
		else flags &= ~(M_ALLFLAGS);
	}
	else {
		mapStr_id=nFloatCnt=0;
	}

	if((flags&M_LRUDS) && pSV->IsSegmentsVisible()) {

		irec=pSV->LrudStyle();

		if(!bProfile && (irec&LRUD_SVGMASK)==LRUD_SVGMASK && (irec&LRUD_BOTH)!=LRUD_TICK) {
			PlotSVGMaskLayer();
			irec&=~LRUD_SOLID; //do not plot lrud polys
		}
		else irec&=~LRUD_SVGMASK;

		if(irec&LRUD_BOTH) {
			ixSEG.UseTree(NTA_LRUDTREE);
			//if(ixSEG.NumRecs() && (rayPnt=(RAYPOINT *)malloc((MAX_RAYPOINTS+2)*sizeof(RAYPOINT)))) {
			if(ixSEG.NumRecs()) {
				vRayPnt.clear();
				vPnt.clear();
				if(LRUD_SOLID&irec) PlotLrudPolys(); //Sets iPolysDrawn
				if(LRUD_TICK&irec) PlotLrudTicks((irec&LRUD_STYLEMASK)==LRUD_TICK);
				vRayPnt.clear();
				vPnt.clear();
				//free(rayPnt);
			}
			ixSEG.UseTree(NTA_SEGTREE);
		}
	}

    if(pSV->IsGridVisible()) {
       if(CSegView::m_pGridFormat->wUseTickmarks) PlotTicks();
       else PlotGrid();
    }

	if(!bNoVectors) {

  		if(mapStr_id) {
  			//Plot the unadjusted version of the selected traverse ignoring visibility --
			if(pSV->IsRawTravVisible() && mapStr_flag[mapStr_id-nSTR_1])
			PlotRawTraverse(mapStr_id,flags&pSV->RawTravLabelFlags(),FALSE);
		}
		else if(nFloatCnt && pSV->IsRawTravVisible()) {
  			//Plot the unadjusted version of all floated traverses observing visibility --
			for(irec=nSTR_1;(int)irec<=nSTR_2;irec++)
				if(mapStr_flag[irec-nSTR_1])
					PlotRawTraverse(irec,flags&pSV->RawTravLabelFlags(),TRUE);
		}

		mapSeg=(WORD)-1; //Conditional pen initialization with first vector

		InitMapPen(&SEG_STYLE_BLACK);
		hPenOld=(HPEN)::SelectObject(pvDC->m_hDC,mapPen);
		PolyInit();

		for(irec=pNTN->plt_id1;irec<=pNTN->plt_id2;irec++) {
			if(dbNTP.Go(irec)) {
				ASSERT(FALSE);
				break;
			}
	  		PlotVector(pPLT->vec_id && LineVisible());
	  	}

		PolyFlush();
		bPolyActive=FALSE;
	  	VERIFY(mapPen==(HPEN)::SelectObject(pvDC->m_hDC,hPenOld));
	  	::DeleteObject(mapPen);
	}

	if(pVnodeMap) {
		if((irec=mapcnt(pVnodeMap,szpVnodeMap))) InitVnode(irec);
		free(pVnodeMap);
		pVnodeMap=NULL;
	}
  	
  	//Stations are labeled and/or marked in a separate pass to avoid being overwritten --
  	irec=0;
	if(flags&M_ALLFLAGS) {
	    if(flags&M_MARKERS) PlotMarkers(flags);
		if((flags&(M_PRINT+M_METAFILE))==M_PRINT) FixClipRegion(0,0);
		if(flags&M_FLAGS) irec=PlotFlags();
		if(flags&M_NOTES) PlotNotes(flags);
		if(flags&M_NAMES) PlotLabels();
    }
      
   	if(!(flags&M_PRINT)) {

		if(irec) AddFlagVnodes(irec);

		flags |= M_MAPFRAME;
	  	//Restore device context for bitmap (required?) --
	    VERIFY(::SelectObject(dcBmp.m_hDC,hBmpOld));
	    if(hBrushOld) ::SelectObject(dcBmp.m_hDC,hBrushOld);

	    if(!pPV->m_bExternUpdate) {
		  	//Create a new frame for a CMapView --
			pFrame = (CMapFrame *)CWallsApp::m_pMapTemplate->CreateNewFrame(m_pReviewDoc,NULL);
			if(pFrame) {
			  ASSERT(pFrame->IsKindOf(RUNTIME_CLASS(CMapFrame)));
			  pFrame->m_pDoc=m_pReviewDoc;
			  pFrame->m_pVnode=pVnode;
			  pFrame->m_pVnodeF=pVnodeF;
			  pVnode=pVnodeF=NULL;
			  pFrame->SetTransform(flags);
			  pFrame->m_pNext=m_pReviewDoc->m_pMapFrame;
			  m_pReviewDoc->m_pMapFrame=pFrame;
			  //Display and activate the view --
			  pFrame->SetFrameSize(size); //view establishes size
			  CWallsApp::m_pMapTemplate->InitialUpdateFrame(pFrame,m_pReviewDoc);
			  //The above will activate frame and place it at top of stack --
			  //pFrame=m_pReviewDoc->m_pMapFrame;
			  pFrame->InitializeFrame(pBmp, size, xoff, yoff, xscale);
			  pFrame->ShowTitle();
			  pFrame->m_pView->DrawScale();
			  pPV->Enable(IDC_UPDATEFRAME, TRUE);
			}
			else delete pBmp;
	    }
	    else {
			//Invalidate the existing CMapView and bring it to the top --

			ASSERT((!pFrame->m_pVnode || CMapFrame::m_bKeepVnode) && !pFrame->m_pVnodeF);
			ASSERT(!CMapFrame::m_bKeepVnode || !pVnode || pVnode->IsInitialized());

			if(!CMapFrame::m_bKeepVnode) {
				  pFrame->m_pVnode=pVnode;
				  pVnode=NULL;
			}
			else CMapFrame::m_bKeepVnode=FALSE;
			pFrame->m_pVnodeF=pVnodeF;
			pVnodeF=NULL;
			pFrame->m_pFrameNode=m_pReviewNode; //***Missing before 5/1/2016!
			pFrame->m_dwReviewNetwork=dbNTN.Position();
			pFrame->SetTransform(flags); //also sets m_bProfile, m_bFeetUnits in view
			pFrame->ShowTitle();
			if(!bKeepScale)	pFrame->m_pView->DrawScale();
			pFrame->m_pView->Invalidate();
			if(!m_bRefreshing) {
				if(pFrame->IsIconic()) pFrame->ShowWindow(SW_RESTORE);
				else pFrame->BringWindowToTop();
			}
	    }
	}
	
	//Restore global required for plotting map panel --
_restore:
	xscale=sav_xscale;
	yscale=sav_yscale;
	xoff=sav_xoff;
	yoff=sav_yoff;

	//Changed only when frame zooming --
	if(pPV->m_bExternUpdate>1) {
		sinA=sav_sinA;
		cosA=sav_cosA;
		fView=sav_fView;
		midU=sav_midU;
		midE=sav_midE;
		midN=sav_midN;
		bProfile=sav_bProfile;
	}

	if(pLabelMap) {
	  free(pLabelMap);
	  pLabelMap=NULL;
    }
	delete pVnodeF;
	delete pVnode;
}

static apfcn_v InitPltSegs()
{
	//Create pltseg array to aid fast plotting of strings.
	//Only called when homing to a new component --
	memset(piPLT,0,nSTR*sizeof(int));
	nPLTSEGS=0;
  	
  	// Why?? -- ASSERT(!pPLT->vec_id);
  	
	UINT plt_id=pNTN->plt_id1;
	UINT plt_idMax=pNTN->plt_id2;
	int str_id1=pNTN->str_id1;
	
  	for(int s=0;plt_id<=plt_idMax;plt_id++) {
		if(dbNTP.Go(plt_id)) {
		  ASSERT(FALSE);
		  return;
		}
		if(pPLT->str_id) {
			if(!s) {
			    //First string vector of a new segment --
			    s=pPLT->str_id;
			    NewPltSeg(s-str_id1,plt_id-1);
			}
			else {
				ASSERT(s==pPLT->str_id);
			}
		}
		else s=0;
  	}
}

void CPrjDoc::InitViewFormat(VIEWFORMAT *pVF)
{
    // NOTE: In PlotVector(), a point's screen coordinates are obtained 
    // as follows --
    //
    // x=(TransAx(E,N)-xoff)*xscale;
	// y=(-TransAy(E,N,U)-yoff)*yscale;  (yscale==xscale)
	//
	// TransAX(E,N)   = (E-midE)*cosA - (N-midN)*sinA  <plans and profiles>
	// TransAY(E,N,U) = (E-midE)*sinA + (N-midN)*cosA  <plans>
	//					(U-midU) 					   <profiles>
	//
	// Note: (midE,MidN)		-- metric coordinates of center of data range
	//		 (E,N)				-- absolute metric coordinates of station
	//		 (xoff,yoff)		-- transformed metric coordinates of frame's UL corner wrt (MidE,MidN)
	//		 (TransAX,TransAy)	-- transformed metric coordinates of station wrt (MidE,MidN)
	//
	// Hence, for plans, 
	//        xoff + W/2 =  (E'-midE)*cosA - (N'-midN)*sinA
	//		  yoff + H/2 = -(E'-midE)*sinA - (N'-midN)*cosA
	//
	//        E' = midE + (xoff+W/2)*cosA - (yoff+H/2)*sinA
	//        N' = midN - (xoff+W/2)*sinA - (yoff+H/2)*cosA
	//
	// and for profiles reference station (0,0) is located at
	//
	//        X = -midE*cosA + midN*sinA - xoff
	//        Y = midU - yoff
	//
	// in survey units wrt the upper-left frame corner. Hence, the frame 
	// center's offset from station (0,0) is
	//
	//	      RIGHT = midE*cosA - midN*sinA + xoff + W/2
	//		  UP    = midU - yoff - H/2
	//
	// where (W,H) are the box dimensions in survey units.
    
	double w,xo=xoff,yo=yoff;

	if(pVF!=&pPV->m_vfSaved[pPV->m_bProfile]) *pVF=pPV->m_vfSaved[pPV->m_bProfile];
    
	if(pPV->m_bTrackerActive) {
		CPoint posTracker;
		w=pPV->GetTrackerPos(&posTracker)/xscale; 
		xo+=posTracker.x/xscale;
		yo+=posTracker.y/xscale;
	    
	}
	else w=pPV->m_sizeBmp.cx/xscale;
	pVF->fPanelWidth=w;
	w/=2;
	xo+=w;
	yo+=(w*pPV->m_sizeBmp.cy)/pPV->m_sizeBmp.cx;

    if(bProfile) {
		w=-sinA*pVF->fCenterEast-cosA*pVF->fCenterNorth; //zoff-Cz
		xo+=-midN*sinA+midE*cosA; //xoff-Cx
		pVF->fCenterEast=cosA*xo-sinA*w;
		pVF->fCenterNorth=-sinA*xo-cosA*w;
	    pVF->fCenterUp=midU-yo;
    }
    else {	
		pVF->fCenterEast=midE+xo*cosA-yo*sinA;
		pVF->fCenterNorth=midN-xo*sinA-yo*cosA;
	}

	pVF->fPanelView=fView;
	pVF->bProfile=bProfile;
	pVF->iComponent=pPV->m_recNTN;
	pVF->bFlags&=~VF_INCHUNITS;
	pVF->bFlags|=pPV->m_bInches;
}

static apfcn_v FixRange(double *rmin,double *rmax,double *xyz)
{
	double r=TransAx(xyz);
	if(r<rmin[0]) rmin[0]=r;
	if(r>rmax[0]) rmax[0]=r;
	r=-TransAy(xyz);
	if(r<rmin[1]) rmin[1]=r;
	if(r>rmax[1]) rmax[1]=r;
}
	
static apfcn_v GetTravScale()
{
	double rmax[2],rmin[2];
	double xyzraw[3];
    pltseg *ps;
	int end_id,idx;
	BOOL bRaw,bChecked;
	
	idx=piSTR[pPV->m_iTrav-1];
    ASSERT(idx>=pNTN->str_id1);
    VERIFY(!dbNTS.Go(idx));
    
	bRaw=(pSTR_flag&(bProfile?NET_STR_FLTMSKV:NET_STR_FLTMSKH))!=0;
    
	rmin[0]=rmin[1]=DBL_MAX;
	rmax[0]=rmax[1]=-DBL_MAX;
    
    end_id=pSTR->id[0]; //The station id of the string's endpoint
    bChecked=FALSE;
    
    for(idx=piPLT[idx-pNTN->str_id1];idx>0;idx=ps->next) {
	    //Next segment --
	    ps=piPLTSEG+idx-1;
	    if(dbNTP.Go(ps->rec)) break;
	    //Check contiguous range of NTP records --
	    do {
		    if(bRaw==1 && end_id==pPLT->end_id) {
		      bRaw++;
		      memcpy(xyzraw,pPLT->xyz,sizeof(xyzraw));
		    }
		    if(pPLT->end_id) {
		      FixRange(rmin,rmax,pPLT->xyz);
		      bChecked=TRUE;
		    } 
	    }
	    while(!dbNTP.Next() && pPLT->str_id);
    }
    
	if(bRaw) {
		ASSERT(bRaw>1);
		if(pSTR->flag&NET_STR_REVERSE) {
		  ReverseEndpoint(xyzraw);
		  if(end_id) FixRange(rmin,rmax,xyzraw);
		}
		idx=pSTR->vec_id;
	
		while(idx && !dbNTV.Go(idx)) {
		    idx=pVEC->id[1]==end_id;
		    ASSERT(idx || end_id==pVEC->id[0]);
			if(idx) {
					xyzraw[0]-=pVEC->xyz[0];
					xyzraw[1]-=pVEC->xyz[1];
					xyzraw[2]-=pVEC->xyz[2];
			}
			else {
					xyzraw[0]+=pVEC->xyz[0];
					xyzraw[1]+=pVEC->xyz[1];
					xyzraw[2]+=pVEC->xyz[2];
			}
			if(end_id=pVEC->id[1-idx]) FixRange(rmin,rmax,xyzraw);
			idx=pVEC->str_nxt_vc;
		}
	}
	
	ASSERT(bChecked);
	if(!bChecked) return; //Corrupted files?
	
	xscale=((double)pPV->m_sizeBmp.cx/pPV->m_sizeBmp.cy)*(rmax[1]-rmin[1]);
	xoff=rmax[0]-rmin[0];
	if(xoff<xscale) xoff=xscale;
	   
	xoff*=1.6;  //Enlarge a little to include context
	if(xoff<MIN_SCALE_WIDTH) xoff=MIN_SCALE_WIDTH; //All shots vertical?
	
	//xoff == width of frame
	
	xscale=pPV->m_sizeBmp.cx/xoff; //pixels per meter
	//Compute (transormed) survey coordinates of upper left frame corner --
	frame[0].xoff=xoff=(rmin[0]+rmax[0])/2.0 - pPV->m_sizeBmp.cx/(2.0*xscale);
	frame[0].yoff=yoff=(rmin[1]+rmax[1])/2.0 - pPV->m_sizeBmp.cy/(2.0*xscale);
	frame[0].mapMeters=pPV->m_fPanelWidth=pPV->m_sizeBmp.cx/xscale;
	nFRM=1;
}

static apfcn_v GetTrackerScale()
{
	//We are zooming into the tracker rectangle if it is active --
	if(pPV->m_bTrackerActive) {
		CPoint posTracker;
		int width=pPV->GetTrackerPos(&posTracker); 
		
		ASSERT(width>0);
		xoff+=posTracker.x/xscale;
		yoff+=posTracker.y/xscale;
		pPV->m_fPanelWidth=(width*pPV->m_fPanelWidth)/pPV->m_sizeBmp.cx;
		pPV->ClearTracker(FALSE);
	}
	else {
	    //Let's increase the plot scale by 50% --
	    xscale=(PERCENT_ZOOMOUT*pPV->m_fPanelWidth)/100;
	    xoff+=xscale/2;
	    yoff+=(xscale*pPV->m_sizeBmp.cy)/(2*pPV->m_sizeBmp.cx);
	    pPV->m_fPanelWidth-=xscale;
	}
	
	xscale=pPV->m_sizeBmp.cx/pPV->m_fPanelWidth;
	
	if(nFRM==MAX_FRAMES) nFRM--;
	frame[nFRM].xoff=xoff;
	frame[nFRM].yoff=yoff;
	frame[nFRM].mapMeters=pPV->m_fPanelWidth;
	nFRM++;
}

static apfcn_v GetZoomOutScale()
{
	//We are backing out --
	ASSERT(nFRM>0);
	if(nFRM==1) {
	    //There is no previous position to establish. Let's decrease the plot size by 50% --
	    frame[1]=frame[0]; //So we can move tracker to previous position
	    double dInc=(PERCENT_ZOOMOUT*frame[0].mapMeters)/100;
	    frame[0].xoff=(xoff-=dInc/2);
	    frame[0].yoff=(yoff-=(dInc*pPV->m_sizeBmp.cy)/(2*pPV->m_sizeBmp.cx));
	    xscale=pPV->m_sizeBmp.cx/(frame[0].mapMeters=pPV->m_fPanelWidth+=dInc);
	}
	else {
	    //Recover the previous window position --
		nFRM-=2;
		xoff=frame[nFRM].xoff;
		yoff=frame[nFRM].yoff;
	    xscale=pPV->m_sizeBmp.cx/(pPV->m_fPanelWidth=frame[nFRM].mapMeters);
	    nFRM++;
	}
	    
	//Resize tracker to show position we are zooming out from --
	frametyp *pf=frame+nFRM;
	if(pf->mapMeters<pPV->m_fPanelWidth) {
		int last_xoff=(int)((pf->xoff-xoff)*xscale+0.5);
		int last_yoff=(int)((pf->yoff-yoff)*xscale+0.5);
									
		//This sets pPV->m_bTrackerActive to TRUE or FALSE, depending on whether
		//the new frame contains the old frame --
		pPV->SetTrackerPos(last_xoff,last_yoff,
		  last_xoff+(int)(((pf->mapMeters*pPV->m_sizeBmp.cx)/pPV->m_fPanelWidth)+0.5),
		  last_yoff+(int)(((pf->mapMeters*pPV->m_sizeBmp.cy)/pPV->m_fPanelWidth)+0.5));
	}
	else pPV->ClearTracker(FALSE);
}

static apfcn_v RotateView()
{
    // NOTE: In PlotVector(), a point's screen coordinates are obtained 
    // as follows --
    //
    // x=(TransAx(E,N)-xoff)*xscale;
	// y=(-TransAy(E,N,U)-yoff)*yscale;
	//
	// TransAX(E,N)   = (E-midE)*cosA - (N-midN)*sinA  <plans and profiles>
	// TransAY(E,N,U) = (E-midE)*sinA + (N-midN)*cosA  <plans>
	//					(U-midU) 					   <profiles>
	//
	// For plans, we want to maintain invariant the frame's center --
	// 
	//        xoff' =  (E'-midE)*cosA' - (N'-midN)*sinA' - W/2
	//		  yoff' = -(E'-midE)*sinA' - (N'-midN)*cosA' - H/2
	//
	//        E' = midE + (xoff+W/2)*cosA - (yoff+H/2)*sinA
	//        N' = midN - (xoff+W/2)*sinA - (yoff+H/2)*cosA
	//
	// where (E',N',U') are the survey coordinates of the point currently
	// positioned at the center of the frame and (W,H) are the frame dimensions
	// in survey units.
	// 
	// For profiles reference station (0,0) is located at
	//
	//        X = -midE*cosA + midN*sinA - xoff
	//        Y = midU - yoff
	//
	// in survey uints wrt the upper-left frame corner. Hence, the frame 
	// center's offset from station (0,0) is
	//
	//	      RIGHT = midE*cosA - midN*sinA + xoff + W/2
	//		  UP    = midU - yoff - H/2
	//
	// To keep this invariant, we must have
	//        
	//        xoff' = RIGHT - W/2 - midE*cosA' + midN+sinA'
	//				= midE*(cosA-cosA') - midN*(sinA-sinA') + xoff
	//		  yoff' = yoff
	
	double EP,NP,W2=pPV->m_sizeBmp.cx/(2.0*xscale);
	
	if(bProfile) {
		double w=-sinA*pPV->m_vfSaved[1].fCenterEast-cosA*pPV->m_vfSaved[1].fCenterNorth; //zoff-Cz
		double xo=xoff+W2-midN*sinA+midE*cosA; //xoff-Cx
		EP=cosA*xo-sinA*w;
		NP=-sinA*xo-cosA*w;
		sinA=sin(fView*U_DEGREES);
		cosA=cos(fView*U_DEGREES);
		xoff=cosA*(EP-midE)-sinA*(NP-midN)-W2;
		//xoff+=midE*(ca-cosA)-midN*(sa-sinA);
	}
	else {
		double H2=pPV->m_sizeBmp.cy/(2.0*xscale);
		EP = (xoff+W2)*cosA - (yoff+H2)*sinA;
		NP = -(xoff+W2)*sinA- (yoff+H2)*cosA;
		sinA=sin(fView*U_DEGREES);
		cosA=cos(fView*U_DEGREES);
		xoff =  EP*cosA - NP*sinA - W2;
		yoff = -EP*sinA - NP*cosA - H2;
	}
	
	ASSERT(nFRM>0);
	frame[0].xoff=xoff;
	frame[0].yoff=yoff;
	frame[0].mapMeters=pPV->m_sizeBmp.cx/xscale;
	nFRM=1;
}

static apfcn_v GetViewFormatScale(VIEWFORMAT *pVF)
{
    // NOTE: In PlotVector(), a point's screen coordinates are obtained 
    // as follows --
    //
    // x=(TransAx(E,N)-xoff)*xscale;
	// y=(-TransAy(E,N,U)-yoff)*yscale;
	//
	// TransAX(E,N)   = (E-midE)*cosA - (N-midN)*sinA  <plans and profiles>
	// TransAY(E,N,U) = (E-midE)*sinA + (N-midN)*cosA  <plans>
	//					(U-midU) 					   <profiles>
	//
	// Hence, for plans, 
	//        xoff + W/2 =  (E'-midE)*cosA - (N'-midN)*sinA
	//		  yoff + H/2 = -(E'-midE)*sinA - (N'-midN)*cosA
	//
	//        E' = midE + (xoff+W/2)*cosA - (yoff+H/2)*sinA
	//        N' = midN - (xoff+W/2)*sinA - (yoff+H/2)*cosA
	//
	// and for profiles reference station (0,0) is located at
	//
	//        X = -midE*cosA + midN*sinA - xoff
	//        Y = midU - yoff
	//
	// in survey uints wrt the upper-left frame corner. Hence, the frame 
	// center's offset from station (0,0) is
	//
	//	      RIGHT = midE*cosA - midN*sinA + xoff + W/2
	//		  UP    = midU - yoff - H/2
	//
	// where (W,H) are the box dimensions in survey units.
	//
	
	if(fView!=pVF->fPanelView) {
	   fView=pVF->fPanelView;
	   RotateView();
	}
	
	xscale=pPV->m_sizeBmp.cx/(pPV->m_fPanelWidth=pVF->fPanelWidth);
	
	double W2=pPV->m_sizeBmp.cx/(xscale*2.0);
	double H2=pPV->m_sizeBmp.cy/(xscale*2.0);

	if(bProfile) {
		xoff=cosA*(pVF->fCenterEast-midE)-sinA*(pVF->fCenterNorth-midN)-W2;
	    yoff=midU-pVF->fCenterUp-H2;
	}
	else {
	    xoff=(pVF->fCenterEast-midE)*cosA-(pVF->fCenterNorth-midN)*sinA-W2;
	    yoff=-(pVF->fCenterEast-midE)*sinA-(pVF->fCenterNorth-midN)*cosA-H2;
	}
	
	if(nFRM==MAX_FRAMES) nFRM--;
	frame[nFRM].xoff=xoff;
	frame[nFRM].yoff=yoff;
	frame[nFRM].mapMeters=pVF->fPanelWidth;
	nFRM++;
}

void CPrjDoc::GetCoordinate(DPOINT &coord,CPoint point,BOOL bGrid)
{
	double x,y;

    if(bProfile) {
	    if(bGrid) y=(point.y-5)/pGV->m_xscale+pGV->m_yoff;
		else y=(point.y-5)/xscale+yoff;
		coord.y=LEN_SCALE(midU-y);
	}
	else {
		if(bGrid) {
     		x=(point.x-6)/pGV->m_xscale+pGV->m_xoff;
			y=(point.y-5)/pGV->m_xscale+pGV->m_yoff;
		}
		else {
			y=(point.y-5)/xscale+yoff;
     		x=(point.x-6)/xscale+xoff;
		}
		coord.x=LEN_SCALE(x*cosA-y*sinA+midE);
		coord.y=LEN_SCALE(-x*sinA-y*cosA+midN);
	}
}

static apfcn_v PanView()
{
    // NOTE: In PlotVector(), a point's screen coordinates are obtained 
    // as follows --
    //
    // x=(TransAx(E,N)-xoff)*xscale;
	// y=(-TransAy(E,N,U)-yoff)*yscale;
	//
	// TransAX(E,N)   = (E-midE)*cosA - (N-midN)*sinA  <plans and profiles>
	// TransAY(E,N,U) = (E-midE)*sinA + (N-midN)*cosA  <plans>
	//					(U-midU) 					   <profiles>
	//
	// Hence, <new xoff>=TransAx(E',N')-W/2 and <new yoff>=-TransAy(E',N',U')-H/2,
	// where (E',N',U') are the current survey coordinates of the point specifind
	// by pPV->m_pPanPoint and (W,H) are the frame dimensions in survey units.
	
	double org[3];
	
	double xc=pPV->m_pPanPoint->x;
	double yc=pPV->m_pPanPoint->y;
	
	org[0]=(double)((xoff+xc)*cosA-(yoff+yc)*sinA+midE);
	org[1]=(double)(-(xoff+xc)*sinA-(yoff+yc)*cosA+midN);
	org[2]=(double)(-(yoff+yc)+midU);
	
	frame[nFRM-1].xoff=xoff=TransAx(org)-pPV->m_sizeBmp.cx/(2.0*xscale);
	frame[nFRM-1].yoff=yoff=-TransAy(org)-pPV->m_sizeBmp.cy/(2.0*xscale);
}

static apfcn_b GetDfltScale()
{
	// Possibilities --
	// pPV->m_iView==-1			 - Determine optimal view based on value
	//						  	   of pPV->m_bProfile.
	// else (pPV->m_iView==-2)
	//						  	   
	// bProfile!=pPV->m_bProfile - Use optimal scale at opposing plan/profile
	//                             orientation. fView should be unchanged.
		    	
	#define FRM_FACT 1.1
	
	//Compute 0/270 view scale based on component ranges --
				
	double xrng,yrng,zrng;
	BOOL bDef;
		 
	xrng=(pNTN->xyzmax[0]-pNTN->xyzmin[0]);
	midE=pNTN->xyzmin[0]+xrng/2;
	yrng=(pNTN->xyzmax[1]-pNTN->xyzmin[1]);
	midN=pNTN->xyzmin[1]+yrng/2;
	zrng=(pNTN->xyzmax[2]-pNTN->xyzmin[2]);
	midU=pNTN->xyzmin[2]+zrng/2;
			  	
  	if(pPV->m_iView==-1) {
	  switch(CPrjDoc::m_pReviewNode->InheritedView()) {
		// InheritedView = 0 (North|East), 1 (North|West), 2 (North), 3 (East), 4 (West)
		// (xrng<yrng) = TRUE (East is best), FALSE (North is best)
		case 0 : iDefView=(xrng<yrng)?90:0; break;
		case 1 : iDefView=(xrng<yrng)?270:0; break;
		case 2 : iDefView=0; break;
		case 3 : iDefView=90; break;
		case 4 : iDefView=270; break;
		default: ASSERT(FALSE); iDefView=0;
	  }
  	  //if(CPrjDoc::m_pReviewNode->IsWestward()) iDefView=bRotated?270:180;
  	  //else iDefView=bRotated?90:0;
	  fView=(double)iDefView;
	  sinA=sin(fView*U_DEGREES);
	  cosA=cos(fView*U_DEGREES);
	  bDef=TRUE;
  	}
	else bDef=(fView==(double)iDefView);

	if(iDefView==90 || iDefView==270) {
	  xscale=xrng;
	  xrng=yrng;
	  yrng=xscale;
	}
	if(xrng<MIN_SCALE_WIDTH) xrng=MIN_SCALE_WIDTH; //A single vertical shot?
			  	
	xscale=pPV->m_sizeBmp.cx/(xrng*FRM_FACT); //pixels per meter
			  	
	if(pPV->m_bProfile) yrng=zrng;
	if(yrng && (yrng=pPV->m_sizeBmp.cy/(yrng*FRM_FACT))<xscale) xscale=yrng;
			  	

  	bProfile=pPV->m_bProfile;

 	xoff=-pPV->m_sizeBmp.cx/(2.0*xscale);
	yoff=-pPV->m_sizeBmp.cy/(2.0*xscale);

	//Compute (transormed) survey coordinates of upper left frame corner --
	frame[0].xoff=xoff;
	frame[0].yoff=yoff;
	frame[0].mapMeters=pPV->m_fPanelWidth=pPV->m_sizeBmp.cx/xscale;
	nFRM=1;
	return bDef;
}

BOOL CPrjDoc::FindRefStation(int iNext)
{
	int irec,irec_max;

	static int iLastRef=0;

	if(!iNext) {
		irec=pNTN->plt_id1;
		if(memicmp("<REF> ",pNTN->name,5)) {
			//This is an isolated component with just one fixed point
			VERIFY(!dbNTP.Go(irec));
			m_iFindStation=irec;
			m_iFindStationIncr=m_iFindStationCnt=m_iFindStationMax=1;
			return TRUE;
		}
		//First, find all fixed stations in component, setting m_iFindStationMax.
		//Then set m_iFindStation and m_iFindStationCnt to the position of the first
		//TO station whose FROM station has pPLT->end_id==0.
		ASSERT(irec==1);
		ASSERT(dbNTN.Position()==1);
		m_iFindStation=m_iFindStationMax=0;
		irec_max=pNTN->plt_id2+1;
		for(;irec<irec_max;irec++) {
			if(dbNTP.Go(irec)) {
				ASSERT(FALSE);
				return FALSE;
			}
			if(!pPLT->end_id && !m_iFindStationMax++) {
				iLastRef=irec;
				m_iFindStation=pPLT->vec_id?(irec-1):(irec+1);
			}
		}
		ASSERT(m_iFindStationMax);
		m_iFindStationCnt=m_iFindStationIncr=1;
		goto _exit;
	}

	//We're scanning multiple #FIXed points in the First component --
	ASSERT(iLastRef && m_iFindStation && m_iFindStationMax>1);
	ASSERT(iNext==m_iFindStationIncr);
	ASSERT(m_iFindStationCnt>0 && m_iFindStationCnt<=m_iFindStationMax);
	ASSERT(iNext==1 || iNext==-1);

	//In case we've moved to a secondary component --
	irec=dbNTN.Position();
	if(irec!=1) {
		VERIFY(!dbNTN.Go(1));
		irec_max=pNTN->plt_id2+1;
		VERIFY(!dbNTN.Go(irec));
	}
	else irec_max=pNTN->plt_id2+1;

	//Set starting record (minus iNext) and final value of m_iFindStationCnt -- 
	if((m_iFindStationCnt+=iNext)==0) {
		irec=irec_max;
		m_iFindStationCnt=m_iFindStationMax;
	}
	else if(m_iFindStationCnt>m_iFindStationMax) {
		irec=0; //0
		m_iFindStationCnt=1;
	}
	else irec=iLastRef;

	m_iFindStation=iLastRef=0;
	if(iNext<0) irec_max=0;

	while((irec+=iNext)!=irec_max) {
		ASSERT(irec>0);
		if(dbNTP.Go(irec)) {
			ASSERT(FALSE);
			return FALSE;
		}
		if(!pPLT->end_id) {
			iLastRef=irec;
			m_iFindStation=pPLT->vec_id?(irec-1):(irec+1);
			break;
		}
	}

_exit:
	ASSERT(m_iFindStation);
	if(dbNTP.Go(m_iFindStation)) {
		m_iFindStation=0;
		ASSERT(FALSE);
	}
	return m_iFindStation!=0;
}

BOOL CPrjDoc::FindStation(const char *pName,BOOL bMatchCase,int iNext /*=0*/)
{
	if(!*pName) {
	   CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_EMPTYNAME);
	   return FALSE;
	}

	if((iNext==-1 || iNext==1) && iNext!=m_iFindStationIncr) {
		//When reversing, find last matching station --
		m_iFindStationIncr=iNext;
		if(dbNTP.Go(m_iFindStation)) {
			ASSERT(FALSE);
			m_iFindStation=0;
			return FALSE;
		}
		return TRUE;
	}

	if(!stricmp(pName,"<REF>")) return FindRefStation(iNext);

	int irec;

 	LPBYTE p=(LPBYTE)pName;
	int pfx,incr,recfirst,reclast;
	char name[NET_SIZNAME];

   if((pfx=TrimPrefix(&p,bMatchCase))==-2) return FALSE;
    GetFldFromStr(name,(LPSTR)p,NET_SIZNAME);
    
    int (*compare)(const void *,const void *,UINT);
    compare=bMatchCase?memcmp:memicmp;
		
	if(iNext>1) {
		//Search current component only and require a single match --
		recfirst=pNTN->plt_id1-1;
		reclast=pNTN->plt_id2+1;
		iNext=0;
	}
	else {
		recfirst=0;
		reclast=dbNTP.NumRecs()+1;
	}

	ASSERT(!iNext || (m_iFindStation>0 && m_iFindStation<reclast));

	if(iNext>=0) incr=1;
	else {
		incr=recfirst;
		recfirst=reclast;
		reclast=incr;
		incr=-1;
	}

	if(iNext) {
		ASSERT(pfx<0);
		ASSERT(m_iFindStationMax>1);
		ASSERT(m_iFindStationCnt>0);
		irec=m_iFindStation;
		if((m_iFindStationCnt+=incr)>m_iFindStationMax) {
			irec=recfirst;
			m_iFindStationCnt=1;
		}
		else if(!m_iFindStationCnt) {
			irec=recfirst;
			m_iFindStationCnt=m_iFindStationMax;
		}
	}
	else {
		irec=recfirst;
		m_iFindStation=m_iFindStationCnt=m_iFindStationMax=0;
		m_iFindStationIncr=1;
	}

	while((irec+=incr)!=reclast) {
		if(dbNTP.Go(irec)) {
			ASSERT(FALSE);
			goto _nofind;
		}
		if(pPLT->flag!=NET_REVISIT_FLAG && !compare(pPLT->name,name,NET_SIZNAME) &&
			(pfx<0 || pfx==pPLT->end_pfx)) {
			if(!iNext) {
				if(pfx>=0) {
					m_iFindStationMax=m_iFindStationCnt=m_iFindStationIncr=1;
					m_iFindStation=irec;
					return TRUE;
				}
				if(++m_iFindStationCnt==1) m_iFindStation=irec;
				continue;
			}
			m_iFindStation=irec;
			return TRUE;
		}
	}
    
	ASSERT(!iNext);
	if(!iNext) {
		m_iFindStationMax=m_iFindStationCnt;
		if(m_iFindStation) {
			m_iFindStationCnt=1;
			if(dbNTP.Go(m_iFindStation)) {
				ASSERT(FALSE);
				m_iFindStation=0;
			}
		    return (m_iFindStation!=0);
		}
	}

_nofind:
	m_iFindStation=0;
	CMsgBox(MB_ICONASTERISK,IDS_ERR_MAPLOCATE1,pName);
	return FALSE;
}
	
BOOL CPrjDoc::GetRefOffsets(const char *pName,BOOL bMatchCase)
{
	//Called from CViewDlg::CheckData() --

	if(!strcmp(pName,"<REF>")) {
		memset(FixXYZ,0,sizeof(FixXYZ));
		return TRUE;
	}

	//Look only in current component. We allready know it's not the component's datum
	//as a result of comparing it with CViewDlg::m_pNameOfOrigin --
    if(!FindStation(pName,bMatchCase,2)) return FALSE;
	if(m_iFindStationMax>1) {
		CString buf;
		GetPrefixedName(buf,-1);
		CMsgBox(MB_ICONINFORMATION,IDS_ERR_MULTIPLEMATCH,(LPCSTR)buf);
		return FALSE;
	}
	memcpy(FixXYZ,pPLT->xyz,sizeof(FixXYZ));
	return TRUE;
}

void CPrjDoc::LocateStation()
{
	//Called from CPlotView::OnFileFind() or OnFindNext() --
	#define FIND_WIDTH 26
	
	int find_height=(int)(((DWORD)pPV->m_sizeBmp.cy*FIND_WIDTH)/pPV->m_sizeBmp.cx);

	memcpy(FixXYZ,pPLT->xyz,sizeof(FixXYZ));
	
    int x=(int)((TransAx(FixXYZ)-xoff)*xscale-FIND_WIDTH/2.0);
    int y=(int)((-TransAy(FixXYZ)-yoff)*xscale-find_height/2.0);
	
    pPV->ClearTracker();
	pPV->SetTrackerPos(x,y,x+FIND_WIDTH,y+find_height);
	
	if(!pPV->m_bTrackerActive) {
		//Pan image --
		DPOINT point((x+FIND_WIDTH/2.0)/xscale,(y+find_height/2.0)/xscale);
		pPV->m_pPanPoint=&point;
		pCV->PlotTraverse(-2);
		pPV->m_pPanPoint=NULL;
		x=(pPV->m_sizeBmp.cx-FIND_WIDTH+1)/2+1;
		y=(pPV->m_sizeBmp.cy-find_height+1)/2+1;
		pPV->SetTrackerPos(x,y,x+FIND_WIDTH,y+find_height);
		if(!pPV->m_bTrackerActive) return;
	}
	
    CRect rect;
	pPV->m_tracker.GetTrueRect(rect);
	pPV->InvalidateRect(&rect);
	pPV->DisplayPanelWidth();
}

static apfcn_v RenderBkg(int i)
{
    //Regenerate background bitmap --
    //i==1/-1 - Tracker zoom in/out
	//i==-2   - Rescale
	//
	//Globals used for scaling --
	//  xoff,yoff     - Offset in survey units of upper left of bitmap
	//  xscale,yscale - pixels per meter (# dev units when mult by survey distance)
	UINT irec;
	BOOL bNewNet=nSTR && bInitialView; //TRUE if new component containing loops
	BOOL bDef=pPV->m_iView==-1;		   //TRUE if default view is requested
	BOOL bSaveButtonActive=TRUE;
	  	
    if(i==1) {
		if(pPV->m_pPanPoint) PanView();
		GetTrackerScale();
		ASSERT(!bDef);
	}
    else if(i==-1) {
		if(pPV->m_pPanPoint) PanView();
		GetZoomOutScale();
		ASSERT(!bDef);
	}
    else {
		ASSERT(i==-2);
		if(bDef || bProfile!=pPV->m_bProfile) {
			//Default view processing --

			//determine default scale and also orient if bDef==(pPV->m_iView==-1)==TRUE.
			bDef=GetDfltScale(); //sets bProfile=pPV->m_bProfile

			//If bDef==TRUE, fView is now set to default orientation,
			//If bDef==FALSE, this is a plan/profile switch where fView is unchanged.

			if(bInitialView && pPV->m_vfSaved[bProfile].bActive) {
				ASSERT(bDef);
				if(pPV->m_vfSaved[bProfile].iComponent) {
					//Set scale and view based on pVF->fPanelView and pVF->m_fPanelWidth --
					GetViewFormatScale(&pPV->m_vfSaved[bProfile]);
					bDef=FALSE; //No longer positioned at default view
				}
				bSaveButtonActive=FALSE; //No need for Save/Recall
			}
			else {
				if(!pPV->m_vfSaved[bProfile].iComponent) bSaveButtonActive=FALSE;
			}

			bDefault=bDef;

			pPV->EnableDefaultButtons(!bDefault);
			//pPV->EnableRecall(bSaveButtonActive);
			pCV->m_bZoomedTrav=FALSE;
       }
       else if(pPV->m_iView>=0) {
			fView=(double)pPV->m_iView;
			RotateView();
       }
       else if(pPV->m_pPanPoint) PanView();
       else if(pPV->m_pViewFormat) GetViewFormatScale(pPV->m_pViewFormat);
       
       if(pCV->m_bHomeTrav) {
         //Invoked from CCompView::OnHomeTrav() --
         if(bNewNet) {
            InitPltSegs();
            bNewNet=FALSE;
         }
         GetTravScale();
         pCV->m_bZoomedTrav=bSaveButtonActive=TRUE;
         bDef=FALSE;
       }
       pPV->ViewMsg(); //Uses global fView and sets pPV->m_iView=-2;
	   pPV->ClearTracker(FALSE);
    }
    
    yscale=xscale;
    
	if(bDefault!=bDef) pPV->EnableDefaultButtons(!(bDefault=bDef));

	if(!pPV->m_pViewFormat) //&& (bInitialView || !bSaveButtonActive || pPV->m_vfSaved[bProfile].bActive))
	 pPV->EnableRecall(bSaveButtonActive);

	pvDC->PatBlt(0,0,pPV->m_sizeBmp.cx,pPV->m_sizeBmp.cy,PATCOPY);
	
	//Update CPlotView scale indicators --
	pPV->m_xscale=xscale;
	pPV->DisplayPanelWidth();
  	
  	if(dbNTP.Go(irec=pNTN->plt_id1)) {
  	  ASSERT(FALSE);
  	  return;
  	}
  	
  	if(bNewNet) {
  	  //Prepare for creating pltseg array to aid plotting of strings --
  	  memset(piPLT,0,nSTR*sizeof(int));
  	  nPLTSEGS=0;
  	}
  	
  	ASSERT(!pPLT->vec_id); //First point in component is surely a MoveTo.
  	
  	CPen *pen=NULL;
  	for(int s=0;irec<=pNTN->plt_id2;irec++) {
  	  if(pPLT->vec_id) {
  	    //LineTo --
  	    if(pPLT->str_id) {
  	      if(!s) {
  	        s=pPLT->str_id;
  	        if(bNewNet) NewPltSeg(s-pNTN->str_id1,irec-1);
  	      }
  	      ASSERT(s==pPLT->str_id);
  	    }
  	    else s=0;
  	    
  	    if(s>0) {
  	      if(!pen) pen=pvDC->SelectObject(&CReView::m_PenSys);
  	    }
  	    else if(pen) {
  	      pvDC->SelectObject(pen);
  	      pen=NULL;
  	    }
  	    PlotVector(1);
  	  }
  	  else {
  	    //MoveTo --
  	  	PlotVector(0);
  	    s=0;
  	  }
  	  if(dbNTP.Next()) break;
  	}
  	
  	if(pen) pvDC->SelectObject(pen);
}

void CPrjDoc::PlotTraverse(int iSys,int iTrav,int iZoom)
{

	//Create or update CPlotView's map panel. iSys and iTrav are the currently selected
	//system and traverse. If dbNTN is positioned at CPlotView::m_recNTN, the
	//component has not changed. Otherwise, iZoom is ignored (set to -2) and the default
	//plan view is generated. A change of view type (plan or profile) or view angle
	//also forces iZoom to -2. The meaning of iZoom:
	  
	//iZoom = -2	  - Change of view. If pPV->m_iView==-1, we obtain optimal view
	//				    while observing pPV->m_bProfile 
	//iZoom = -1      - Zoom out to a previous tracker position, or to a larger scale.
	//iZoom = +1      - Zoom in to the tracker position
	//iZoom =  0      - Repaint bitmap *only* if this is a traverse zoom (pCV->m_bHomeTrav)

	ASSERT(dbNTN.Opened() && dbNTS.Opened() && dbNTP.Opened());
	  
	if((int)dbNTN.Position()!=pPV->m_recNTN) {
		bInitialView=TRUE;
		pPV->LoadViews(); //Also initializes pPV->m_bProfile
		pPV->m_iView=-1;
		CPrjDoc::m_bPageActive=FALSE;
		iZoom=-2;
	}
	else {
		bInitialView=FALSE;

		if(pCV->m_bHomeTrav					//Homing to a selected traverse
			|| pCV->m_iStrFlag==-2			//Float status of current traverse has changed 
			|| pPV->m_iView>=0				//New orientation only
			|| bProfile!=pPV->m_bProfile	//This is a plan/provile switch
		) iZoom=-2;
		//else iZoom==0 (no RenderBkg) or +-1 (zoom)
	}
	
	//iSys and iTrav are listbox indices of selected system and selected traverse.
	if(iTrav>=0) {
		if(iSys<0) {
		   ASSERT(nISOL);
		   iSys=nSYS;
		}
	    if(iSys) iTrav+=piSYS[iSys-1]; //iTrav=index in piSTR of traverse
	}
	else iSys=-1;
	
	BOOL bNewSys=iSys>=0 && (iZoom || iSys!=pPV->m_iSys-1);
	  
	if(!bNewSys && !iZoom && iTrav==pPV->m_iTrav-1 &&
	  (iStrFlag&(NET_STR_FLTMSK+NET_STR_REVERSE))==
	  (pCV->m_iStrFlag&(NET_STR_FLTMSK+NET_STR_REVERSE))) return;
	
	int iSysOld=pPV->m_iSys-1;
	pPV->m_iSys=iSys+1;
	pPV->m_iTrav=iTrav+1;
	 
	CDC dcBkg;
	CBitmap *pBmpOld;
	CPen *pPenOld;
	HBRUSH hBrushOld;
	  
	VERIFY(dcBkg.CreateCompatibleDC(NULL));
	VERIFY(pBmpOld=dcBkg.SelectObject(&pPV->m_cBmpBkg));
	VERIFY(pPenOld=dcBkg.SelectObject(&CReView::m_PenNet));
    VERIFY(hBrushOld=(HBRUSH)::SelectObject(dcBkg.m_hDC,CReView::m_hPltBrush));
    
	pvDC=&dcBkg;
	
	iSizeFixBox=3;
	bPolyActive=FALSE; //May change this later!

	mapSize=pPV->m_sizeBmp; //Used for vector clipping
	SetClipRange(0,0);
	  
	if(iZoom) {
		bChkRange=FALSE;
		RenderBkg(iZoom);
		pPV->InvalidateRect(&pPV->m_rectBmp,FALSE);
	}
	else {
		bChkRange=TRUE;
		hbrBoxDrawn=NULL;
		xmin=ymin=INT_MAX;
		xmax=ymax=INT_MIN;
	} 
  
	//Next, highlight the selected loop system if iZoom!=0 or if it has changed.
	if(bNewSys) {
		if(!iZoom) {
		    //Remove highlight from previous system --
		    ASSERT(iSysOld>=0);
		    pvDC->SelectObject(&CReView::m_PenSys);
		    if(pPV->m_iSys) PlotSystem(iSysOld);
		}
		//Highlight the selected system --
		pvDC->SelectObject(&CReView::m_PenSysSel);
		PlotSystem(iSys);
		if(bChkRange) InvalidateRange(FALSE);
	}
	
	//Copy all but the highlighted traverse to the main bitmap that we will display --  
    VERIFY(pPV->m_dcBmp.BitBlt(0,0,pPV->m_sizeBmp.cx,pPV->m_sizeBmp.cy,&dcBkg,0,0,SRCCOPY));
    
    //We are finished with the background device context --
	dcBkg.SelectObject(pPenOld);
	dcBkg.SelectObject(pBmpOld);
    ::SelectObject(dcBkg.m_hDC,hBrushOld);
    dcBkg.DeleteDC();
    
    pvDC=&pPV->m_dcBmp;
    
	iStrFlag=-1;
    if(iSys>=0) {
        BOOL bInvalidateTrav=bChkRange;
		bChkRange=TRUE;
		hbrBoxDrawn=NULL;
		xmin=ymin=INT_MAX;
		xmax=ymax=INT_MIN;
		//Highlight the selected string --
		pPenOld=pvDC->SelectObject(&CReView::m_PenTravSel);
		PlotString(iTrav,TRUE); //Sets iStrFlag
		pvDC->SelectObject(pPenOld);
		bChkRange=bInvalidateTrav;
		InvalidateRange(TRUE);
	}
	pCV->m_iStrFlag=iStrFlag;
}

static apfcn_v GetRectViewCell(CRect *pRect,int ptx,int pty)
{

	double x=(pGV->m_fPanelWidth-pGV->m_fViewWidth)/2.0+ptx*pGV->m_fViewWidth-pGV->m_xpan;
	double y=(pGV->m_fPanelHeight-pGV->m_fViewHeight)/2.0+pty*pGV->m_fViewHeight-pGV->m_ypan;

	pRect->SetRect((int)(x*xscale+0.5),(int)(y*yscale+0.5),
			(int)((x+pGV->m_fViewWidth)*xscale+0.5)+1,(int)((y+pGV->m_fViewHeight)*yscale+0.5)+1);
}

static apfcn_b PlotSelectedCells()
{
	int i,j;
	CPoint *pt;
	CRect rect;
	BOOL bCenterSelected=FALSE;
	CBrush *brush=(CBrush *)pvDC->SelectStockObject(WHITE_BRUSH);
	CPen *pOldPen=(CPen *)pvDC->SelectStockObject(NULL_PEN);

	for(i=0;i<(int)pGV->m_numsel;i++)
		if((j=pGV->m_selseq[i])>0) {
		  pt=&pGV->m_selpt[j-1];
		  if(pt->x || pt->y) {
			  GetRectViewCell(&rect,pt->x,pt->y);
			  pvDC->Rectangle(&rect);
		  }
		  else bCenterSelected=TRUE;
		}

    pvDC->SelectObject(pOldPen);
	pvDC->SelectObject(brush);
	return bCenterSelected;
}


static apfcn_v PlotCenterPage(CRect *pRect,BOOL bSelected)
{
	double px,py,x,y,scale;

	//page dimensions in meters --
	scale=(pGV->m_fViewWidth/pGV->m_fFrameWidth);
	px=((double)pGV->m_fInchesX)*scale;
	py=((double)pGV->m_fInchesY)*scale; 

	//upper left corner of page
	{
		VIEWFORMAT *pVF=&pPV->m_vfSaved[bProfile];

		if(pVF->fPageOffsetX!=FLT_MIN) {
			x=(pGV->m_fPanelWidth-pGV->m_fViewWidth)/2.0-pVF->fPageOffsetX*scale;
			y=(pGV->m_fPanelHeight-pGV->m_fViewHeight)/2.0-pVF->fPageOffsetY*scale;
		}
		else {
			x=(pGV->m_fPanelWidth-px)/2.0;
			y=(pGV->m_fPanelHeight-py)/2.0;
		}
	}

	x-=pGV->m_xpan;
	y-=pGV->m_ypan;

	pRect->SetRect((int)(x*xscale+0.5),(int)(y*yscale+0.5),
		(int)((x+px)*xscale+0.5)+1,(int)((y+py)*yscale+0.5)+1);

	pvDC->Rectangle(pRect);

}

static void PlotPageOutlines()
{
	int x,y;
	double d0;

	double dxInc=pGV->m_fViewWidth;
	double dyInc=pGV->m_fViewHeight;
	
	//Draw vertical lines --
	d0=pGV->m_pvxoff-xoff-(int)((pGV->m_pvxoff-xoff)/dxInc)*dxInc;
	if(d0<=0.0) d0+=dxInc;
	y=(int)(pGV->m_fPanelHeight*yscale);
	while(d0<pGV->m_fPanelWidth) {
		x=(int)(d0*xscale+0.5);
		pvDC->MoveTo(x,0);
		pvDC->LineTo(x,y);
		d0+=dxInc;
	}

	//Draw horizontal lines --
	d0=pGV->m_pvyoff-yoff-(int)((pGV->m_pvyoff-yoff)/dyInc)*dyInc;
	if(d0<=0.0) d0+=dyInc;
	x=(int)(pGV->m_fPanelWidth*yscale);
	while(d0<pGV->m_fPanelHeight) {
		y=(int)(d0*xscale+0.5);
		pvDC->MoveTo(0,y);
		pvDC->LineTo(x,y);
		d0+=dyInc;
	}
}

static apfcn_v RenderSurveys()
{
    //Regenerate background bitmap --
    //i==1/-1 - Tracker zoom in/out
	//i==-2   - Rescale
	//
	//Globals used for scaling --
	//  xoff,yoff     - Offset in survey units of upper left of bitmap
	//  xscale,yscale - pixels per meter (# dev units when mult by survey distance)

	UINT irec;

    //Fill bitmap with brush color --
	pvDC->PatBlt(0,0,pGV->m_sizeBmp.cx,pGV->m_sizeBmp.cy,BLACKNESS);

	bChkRange=FALSE;
  	
  	if(dbNTP.Go(irec=pNTN->plt_id1)) {
  	  ASSERT(FALSE);
  	  return;
  	}
  	
  	ASSERT(!pPLT->vec_id); //First point in component is surely a MoveTo.
  	
  	for(;irec<=pNTN->plt_id2;irec++) {
  	  if(pPLT->vec_id && pSV->SegNode(pPLT->seg_id)->IsVisible()) {
  	    //LineTo --
	    PlotVector(1);
  	  }
  	  else {
  	    //MoveTo --
  	  	PlotVector(0);
  	  }
  	  if(dbNTP.Next()) break;
  	}
}

void CPrjDoc::PlotPageGrid(int iZoom)
{

	//Create or update page grid. Current view is based on CPlotView settings.
	//The meaning of iZoom:
	//iZoom = -2	  - Change of view. If pPV->m_iView==-1, we obtain optimal view
	//				    while observing pPV->m_bProfile
	//iZoom = -3      - Panning from CPageView::OnMouseMove()
	//iZoom =  4      - After CWallsApp::OnFilePrintSetup()
	//iZoom =  2      - After CPageView::OnLButtonDblClk()
	//iZoom = -1      - Zoom out to a previous tracker position, or to a larger scale.
	//iZoom = +1      - Zoom in to the tracker position
	//iZoom =  0      - Repaint bitmap *only* if there has been a preview map view change
	//				  - or if this is the first invocation --
	//                  CPageView::OnRButtonDown()
	//					CPageView::OnSelectAll(), CPageView::OnClearAll()
	//					CReView::switchTab(), CPlotView::OnChgView()

	CDC dcBkg;
	CBitmap *pBmpOld;
	CPen *pPenOld;
	CBrush *pBrushOld;
	//CPen pen(PS_SOLID,1,RGB(128,128,128));
	CPen pen(PS_SOLID,1,RGB(192,192,192));
	double s_xoff=xoff,s_yoff=yoff,s_xscale=xscale;

	ASSERT(xscale==yscale);
	pvDC=&dcBkg;
	  
	VERIFY(dcBkg.CreateCompatibleDC(NULL));
	VERIFY(pBmpOld=dcBkg.SelectObject(&pGV->m_cBmpBkg));
	VERIFY(pPenOld=dcBkg.SelectObject(&pen));
//  VERIFY(pPenOld=dcBkg.SelectObject(&CReView::m_PenNet));
//	VERIFY(hBrushOld=(HBRUSH)::SelectObject(dcBkg.m_hDC,::GetStockObject(NULL_BRUSH)));
  	
	iSizeFixBox=3;
	bPolyActive=FALSE;

	mapSize=pGV->m_sizeBmp; //Used for vector clipping
	SetClipRange(0,0);

	pMF=&CPlotView::m_mfPrint;
	BOOL bNewFrame=FALSE;

	if(iZoom!=-3) {
		//Not panning --

		if(!CPrjDoc::m_bPageActive ||
			iZoom==-2 ||
			xoff!=pGV->m_pvxoff ||
			yoff!=pGV->m_pvsyoff ||
			fView!=pGV->m_pvview ||
			xscale!=pGV->m_pvxscale ||
			(bNewFrame=(
			  (pGV->m_fViewWidth!=pPV->m_fPanelWidth) ||
			  (pGV->m_fFrameHeight!=pPV->m_vfSaved[bProfile].fFrameHeight) ||
			  (pGV->m_fFrameWidth!=pPV->m_vfSaved[bProfile].fFrameWidth)
			  )))
		{
			pGV->m_fViewWidth=pPV->m_fPanelWidth;
			pGV->m_fFrameHeight=pPV->m_vfSaved[bProfile].fFrameHeight;
			pGV->m_fFrameWidth=pPV->m_vfSaved[bProfile].fFrameWidth;
			pGV->m_fViewHeight=(pGV->m_fFrameHeight/pGV->m_fFrameWidth)*pPV->m_fPanelWidth;

			//Compute increase to panel width
			if(!bNewFrame && (!CPrjDoc::m_bPageActive || xscale!=pGV->m_pvxscale || fView!=pGV->m_pvview || iZoom==-2)) {
				pGV->m_fPanelWidth=(1.0+PAGE_ZOOMOUT/100.0)*pPV->m_fPanelWidth;
				pGV->m_fPanelHeight=(mapSize.cy*pGV->m_fPanelWidth)/mapSize.cx;
				if(pGV->m_fPanelHeight/pGV->m_fViewHeight < (1.0+PAGE_ZOOMOUT/100.0)) {
					pGV->m_fPanelHeight=(1.0+PAGE_ZOOMOUT/100.0)*pGV->m_fViewHeight;
					pGV->m_fPanelWidth=(mapSize.cx*pGV->m_fPanelHeight)/mapSize.cy;
				}
				pGV->m_pvxscale=xscale;
				pGV->m_pvview=fView;
				pGV->m_xpan=pGV->m_ypan=0.0;
			}

			pGV->m_pvxoff=xoff;
			pGV->m_pvsyoff=yoff;
			pGV->RefreshPageInfo();
			CPrjDoc::m_bPageActive=TRUE;
			if(iZoom!=2) {
			  pGV->m_numsel=0;
			  pGV->SelectCell(0,0);
			}
			iZoom=-2;
		}
		else if(iZoom==-1 || iZoom==1) {
			if(iZoom<0) pGV->m_fPanelWidth*=(1.0+PERCENT_ZOOMOUT/100.0);
			else pGV->m_fPanelWidth/=(1.0+PERCENT_ZOOMOUT/100.0);
			pGV->m_fPanelHeight=(mapSize.cy*pGV->m_fPanelWidth)/mapSize.cx;
		}

		if(iZoom==4) {
			pGV->RefreshPageInfo();
			iZoom=0;
		}
		else if(iZoom) {
				pGV->m_xoff=xoff+(pPV->m_fPanelWidth-pGV->m_fPanelWidth)/2.0;
				pGV->m_yoff=yoff+((pPV->m_fPanelWidth*pPV->m_sizeBmp.cy)/pPV->m_sizeBmp.cx-
								   pGV->m_fPanelHeight)/2.0;

				pGV->m_xscale=mapSize.cx/pGV->m_fPanelWidth;
				pGV->m_pvyoff=pGV->m_yoff+(pGV->m_fPanelHeight-pGV->m_fViewHeight)/2.0;
		}
	} //iZoom!=-3

	xoff=pGV->m_xoff+pGV->m_xpan;
	yoff=pGV->m_yoff+pGV->m_ypan;
	xscale=yscale=pGV->m_xscale;

	//If necessary, regenerate background in dcBkg--
	if(iZoom || pSV->IsNewVisibility()) RenderSurveys();
	dcBkg.SelectObject(pPenOld);
  
	//Now draw the selected pages --
	pvDC=&pGV->m_dcBmp;
    //Fill bitmap with LTGRAY brush color --
	pvDC->PatBlt(0,0,pGV->m_sizeBmp.cx,pGV->m_sizeBmp.cy,PATCOPY);

	CRect viewRect,pageRect;
	BOOL bCenterSel=PlotSelectedCells();

	//Copy background to the main bitmap that we will display --  
    VERIFY(pvDC->BitBlt(0,0,pGV->m_sizeBmp.cx,pGV->m_sizeBmp.cy,&dcBkg,0,0,SRCINVERT));

	//Plot grid lines --
    VERIFY(pBrushOld=(CBrush *)pvDC->SelectStockObject(NULL_BRUSH));
	VERIFY(pPenOld=pvDC->SelectObject(&CReView::m_PenNet));
	PlotPageOutlines();

	//Plot rectangle for printable area --
	pvDC->SelectStockObject(bCenterSel?WHITE_BRUSH:NULL_BRUSH);
	pvDC->SelectStockObject(NULL_PEN);
	PlotCenterPage(&pageRect,bCenterSel);

	if(bCenterSel) {
	//Copy background to the view frame inside printable area --
	GetRectViewCell(&viewRect,0,0);
    VERIFY(pvDC->BitBlt(viewRect.TopLeft().x,viewRect.TopLeft().y,
		viewRect.Width(),viewRect.Height(),&dcBkg,viewRect.TopLeft().x,viewRect.TopLeft().y,SRCINVERT));

	//Draw view frame --
	pvDC->SelectStockObject(NULL_BRUSH);
	pvDC->SelectStockObject(BLACK_PEN);
    pvDC->Rectangle(&viewRect);
	}

	//Finally, draw blue rectangle around printable area --
    VERIFY(pvDC->SelectObject(&CReView::m_PenSysSel));
    pvDC->Rectangle(&pageRect);

	pvDC->SelectObject(pPenOld);
	pvDC->SelectObject(pBrushOld);

	pGV->InvalidateRect(&pGV->m_rectBmp,FALSE);

    pvDC=&pPV->m_dcBmp;

    //We are finished with the background device context --
	dcBkg.SelectObject(pBmpOld);
    dcBkg.DeleteDC();

	xoff=s_xoff;
	yoff=s_yoff;
	xscale=yscale=s_xscale;
}

static apfcn_v AdvancePt(CPoint& pt,double x0,double y0,double x,double y)
{
	double slope=(y-y0)/(x-x0);
	BOOL bToLeft=(x<x0);

	x=pGV->m_pvxoff+pt.x*pGV->m_fViewWidth;
	y=pGV->m_pvyoff+pt.y*pGV->m_fViewHeight;

	if(bToLeft) {
		if(slope>0.0) {
			if((y-y0)/(x-x0)<slope) pt.y--; else pt.x--;
		}
		else {
			if((y+pGV->m_fViewHeight-y0)/(x-x0)>slope) pt.y++; else pt.x--;
		}
	}
	else {
		x+=pGV->m_fViewWidth;
		if(slope>0.0) {
			if((y+pGV->m_fViewHeight-y0)/(x-x0)<slope) pt.y++; else pt.x++;
		}
		else {
			if((y-y0)/(x-x0)>slope) pt.y--; else pt.x++;
		}
	}
}

static BOOL SelectVectorCells(int bVec_id)
{
	double x,y;
	CPoint pt;
	static double x0,y0;
	static CPoint pt0;
	static BOOL bLastMove;

	//bVec_id==0 - MoveTo
	//bVec_id==1 - DrawTo

	if(!pPLT->end_id) return bLastRef=TRUE;

	x=TransAx(pPLT->xyz);
	y=-TransAy(pPLT->xyz);
	pGV->GetCell(&pt,x,y);

	if(!bVec_id || !pSV->SegNode(pPLT->seg_id)->IsVisible()) {
		bLastMove=TRUE;
		goto _next;
	}

	if(bLastMove && !pGV->SelectCell(pt0)) return FALSE;
	bLastMove=FALSE;

	if(!pGV->SelectCell(pt)) return FALSE;

	if(bVec_id && !bLastRef) {
		//line to --
		//If necessary, select intermediate cells by advancing (x0,y0) toward (x,y)
		while(pt0!=pt) {
			//advance pt0 --
			if(pt0.x==pt.x) pt0.y+=(pt0.y<pt.y)?1:-1;
			else if(pt0.y==pt.y) pt0.x+=(pt0.x<pt.x)?1:-1;
			else {
			  //advance (x0,y0) to frame intersection and set pt0 to adjacent frame --
			  AdvancePt(pt0,x0,y0,x,y);
			}
			if(pt0==pt) break;
			if(!pGV->SelectCell(pt0)) return FALSE;
		}
	}

_next:
	bLastRef=FALSE;
	pt0=pt;
	x0=x;
	y0=y;
	return TRUE;
}

void CPrjDoc::SelectAllPages()
{
	UINT irec;

  	if(dbNTP.Go(irec=pNTN->plt_id1)) {
  	  ASSERT(FALSE);
  	  return;
  	}
  	
    pGV->m_numsel=0;

  	ASSERT(!pPLT->vec_id); //First point in component is surely a MoveTo.
  	
	for(;irec<=pNTN->plt_id2;irec++)
	  if(!SelectVectorCells(pPLT->vec_id!=0) || dbNTP.Next()) break;
}

	
