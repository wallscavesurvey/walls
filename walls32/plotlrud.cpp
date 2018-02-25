#include "stdafx.h"           
#include "walls.h"
#include "ntfile.h"
#include <trxfile.h>
#include "compview.h"
#include "plotview.h"
#include "segview.h"
#include "compile.h"

//Functions in HULL2D.CPP --
UINT GetConvexHull(POINT *pt,UINT ptCnt);
UINT ElimDups(POINT *pt,UINT ptCnt);

static int iGradIdx;
static BOOL bLastLrud;
static COLORREF clrBrush;
static DWORD *pPltRec;
static double *pPltZ;
static DWORD numSeqRec,numPltRec;

static int insert_raypoint(int id,float hz,float vt,double x,double y,double az)
{
	int i=vRayPnt.size();
	if(id+3>i) {
		vRayPnt.resize(i+50);
		vPnt.resize(i+50);
	}

	if(hz==LRUD_BLANK /* || id==MAX_RAYPOINTS*/) return id;
	if(fabs(hz)>=500.0) {
		hz=0;
	}
	az=fmod(az,2*PI);
	if(az<0.0) az+=2*PI;
	for(i=id;i>0;i--) {
		if(az>=vRayPnt[i-1].az) break;
		vRayPnt[i]=vRayPnt[i-1];
	}
	vRayPnt[i].az=az;
	vRayPnt[i].x=x+hz*sin(az);
	vRayPnt[i].y=y+(bProfile?vt:(hz*cos(az)));
	return id+1;
}

int getRayPoints(double x,double y,int id,BOOL bPoly)
{
	char key[6];
	NTA_LRUDTREE_REC lrud;
	double max_up,max_dn;
	float fExag=bPoly?fLrudExag:0.0f;

	*key=3;
	*(UINT *)(key+1)=id;

	id=0;
	if(ixSEG.Find(key)) goto _exit;

	max_up=max_dn=0.0;

	do {
		if(ixSEG.GetRec(&lrud,5*sizeof(float))) {
			ASSERT(trx_errno==CTRXFile::ErrTruncate);
		}
		if(bProfile && (lrud.dim[0]!=LRUD_BLANK_NEG || lrud.az==LRUD_BLANK)) {
			if(lrud.dim[2]!=LRUD_BLANK) max_up=__max(max_up,(double)(lrud.dim[2]+fExag));
			if(lrud.dim[3]!=LRUD_BLANK) max_dn=__max(max_dn,(double)(lrud.dim[3]+fExag));
		}

		if(lrud.az==LRUD_BLANK) {
			ASSERT(lrud.dim[0]==LRUD_BLANK_NEG);
			//This is a vertical ray where we've preserved the verticals for profiles above
			continue;
		}

		double az=(double)lrud.az;

		ASSERT(az<=360.0 && az>=0.0);

		if(lrud.dim[1]!=LRUD_BLANK || (lrud.dim[0]!=LRUD_BLANK && lrud.dim[0]!=LRUD_BLANK_NEG)) {
			//We have at least one ray with a horizontal component
			//and possibly a vertical component --
			az=(az-fView)*(PI/180.0);
			if(lrud.dim[0]==LRUD_BLANK_NEG) {
				float z=0.0;
				if(bProfile && ((z=lrud.dim[2])==LRUD_BLANK || z==0) && (z=-lrud.dim[3])==LRUD_BLANK_NEG) z=0.0;
				id=insert_raypoint(id,lrud.dim[1]+fExag,z,x,y,az);
			}
			else {
				id=insert_raypoint(id,lrud.dim[0]+fExag,0,x,y,az-PI2);
				id=insert_raypoint(id,lrud.dim[1]+fExag,0,x,y,az+PI2);
			}
		}
	}
	while(/*id<MAX_RAYPOINTS && */!ixSEG.FindNext(key));

	if(id) {

#ifdef _DEBUG
		static int max_raypts=0;
		if(id>max_raypts) {
			max_raypts=id;
		}
#endif
		//For now, at least one ray must have a non-zero horiz component --
		vRayPnt[id].x=x;
		vRayPnt[id].y=y;
		if(bProfile) {
			vRayPnt[id+1].x=y+max_up;
			vRayPnt[id+1].y=y-max_dn;
		}
	}

_exit:
	return rayCnt=id;
}

static apfcn_v PlotFan(POINT *pt,int ptCnt)
{
	POINT p[3];

	p[0]=pt[ptCnt]; //center pt
	p[1]=pt[ptCnt-1];
	for(int i=0;i<ptCnt;i++) {
		p[2]=pt[i];
		::Polygon(pvDC->m_hDC,p,3);
		p[1]=p[2];
	}
}

static POINT GetScaledPoint(double x,double y)
{
	POINT pt;

	pt.x=(LONG)(xscale*x);
	pt.y=(LONG)(-yscale*y);
	return pt;
}

static apfcn_v PlotRayPolygon()
{
	RAYPOINT *rp=&vRayPnt[0];
	int r=rayCnt;
	int ptCnt=0;
	BOOL bCorner=FALSE;
	double x,y;
	double lastaz=vRayPnt[rayCnt-1].az-2*PI;
	//POINT pt[MAX_RAYPOINTS+2];

	ASSERT(vPnt.size()>=rayCnt+2);

	while(r) {
		if(!bCorner && (rp->az-lastaz)>(1.01*PI)) {
			bCorner=TRUE;
			x=vRayPnt[rayCnt].x;
			y=vRayPnt[rayCnt].y;
		}
		else {
			lastaz=rp->az;
			x=rp->x;
			y=rp->y;
			rp++;
			r--;
		}
		vPnt[ptCnt++]=GetScaledPoint(x,y);
	}
	if((ptCnt=ElimDups(&vPnt[0],ptCnt))<2 || (ptCnt==2 && !bProfile)) return;

	if(bProfile) {
		vPnt[ptCnt]=GetScaledPoint(vRayPnt[rayCnt].x,y=vRayPnt[rayCnt+1].x);
		PlotFan(&vPnt[0],ptCnt);
		if(y!=vRayPnt[rayCnt+1].y) {
			vPnt[ptCnt].y=(long)(-yscale*vRayPnt[rayCnt+1].y);
			PlotFan(&vPnt[0],ptCnt);
		}
	}
	else {
		::Polygon(pvDC->m_hDC,&vPnt[0],ptCnt);
	}
}

static apfcn_v PlotRays()
{
	UINT ptCnt;
	//POINT pt[MAX_RAYPOINTS+2];
	POINT pt0;

	for(ptCnt=0;ptCnt<rayCnt;ptCnt++) {
		vPnt[ptCnt]=GetScaledPoint(vRayPnt[ptCnt].x,vRayPnt[ptCnt].y);
	}
	ptCnt=ElimDups(&vPnt[0],ptCnt);

	if(ptCnt) {
	  pt0=GetScaledPoint(vRayPnt[rayCnt].x,vRayPnt[rayCnt].y);
      for(UINT i=0;i<ptCnt;i++) {
		  pvDC->MoveTo(pt0);
		  pvDC->LineTo(vPnt[i]);
	  }
	}

	if(bProfile) {
		pvDC->MoveTo(GetScaledPoint(vRayPnt[rayCnt].x,vRayPnt[rayCnt+1].x));
		pvDC->LineTo(GetScaledPoint(vRayPnt[rayCnt].x,vRayPnt[rayCnt+1].y));
	}
}

static apfcn_v get_rotated_pt(POINT *pt,RAYPOINT *rp,double sa)
{
	RAYPOINT *rp0=&vRayPnt[rayCnt];
	double x=rp->x-rp0->x;
	double y=rp->y-rp0->y;
	double ca=cos(sa);

	sa=sin(sa);
	*pt=GetScaledPoint(rp0->x+x*ca+y*sa,bProfile?(rp->y):(rp0->y+y*ca-x*sa));
}

static apfcn_u GetPointsFromRays(POINT *pt,double vec_az)
{
	UINT i,lasti,ptCnt;
	double nextaz,lastaz;

	ptCnt=0;

	if(vec_az==(double)LRUD_BLANK) {
		ASSERT(bProfile);
		lastaz=nextaz=vRayPnt[0].x;
		lasti=0;
		for(i=1;i<rayCnt;i++)
			if(vRayPnt[i].x<lastaz) {
				lastaz=vRayPnt[i].x;
				lasti=i;
			}
			else if(vRayPnt[i].x>nextaz) {
				nextaz=vRayPnt[i].x;
				ptCnt=i;
			}

		i=ptCnt;
		ptCnt=0;
	}
	else {

		if(rayCnt==1) {
			i=0;
			lasti=1;
		}
		else {
			lastaz=vRayPnt[lasti=rayCnt-1].az-2*PI;
			for(i=0;i<rayCnt;i++) {
				ASSERT(vRayPnt[i].az>=lastaz);
				if(vec_az<=vRayPnt[i].az) break;
				lastaz=vRayPnt[lasti=i].az;
			}
			if(i<rayCnt) nextaz=vRayPnt[i].az;
			else nextaz=vRayPnt[i=0].az+2*PI;
			if(rayCnt==2 && fabs(nextaz-lastaz-PI)<0.01) {
				//Use Enlargement factor --
				nextaz=(pSV->LrudEnlarge()/10.0)*(vec_az-lastaz-PI2);
				//nextaz is amount to rotate points CW about station
				if(fabs(nextaz)>=(2.0/180.0)*PI) {
					get_rotated_pt(&pt[0],&vRayPnt[lasti],nextaz);
					get_rotated_pt(&pt[1],&vRayPnt[i],nextaz);
					ptCnt=2;
				}
			}
			else {
				if(vec_az-lastaz>PI) lasti=rayCnt;
				else if(nextaz-vec_az>PI) i=rayCnt;
			}
		}
	}

	if(!bProfile || !ptCnt) {
		pt[ptCnt++]=GetScaledPoint(vRayPnt[lasti].x,vRayPnt[lasti].y);
		if(i!=lasti) {
			pt[ptCnt++]=GetScaledPoint(vRayPnt[i].x,vRayPnt[i].y);
		}
	}

	if(bProfile) {
		pt[ptCnt++]=GetScaledPoint(vRayPnt[rayCnt].x,vRayPnt[rayCnt+1].x);
		pt[ptCnt++]=GetScaledPoint(vRayPnt[rayCnt].x,vRayPnt[rayCnt+1].y);
	}

	return ptCnt;
}

static apfcn_v PlotLrudVector(double x0,double y0,double x1,double y1)
{
	//Draw vector line using gradient color --
	HDC hDC=pvDC->m_hDC;
	POINT pt;
	HPEN hpen=::CreatePen(PS_SOLID,1,pMF->bUseColors?clrGradient:RGB_BLACK);
	ASSERT(hpen);
	if(hpen) {
		hpen=(HPEN)::SelectObject(hDC,hpen);
		pt=GetScaledPoint(x0,y0);
		::MoveToEx(hDC,pt.x,pt.y,NULL);
		pt=GetScaledPoint(x1,y1);
		::LineTo(hDC,pt.x,pt.y);
		::DeleteObject(::SelectObject(hDC,hpen));
	}
	if(pVnodeMap) {
		VERIFY(!mapset(pVnodeMap,dbNTP.Position()-pNTN->plt_id1));
		ASSERT(maptst(pVnodeMap,dbNTP.Position()-pNTN->plt_id1));
		iPolysDrawn++;
	}
}

static BOOL VectorNeeded(styletyp *pstyle,BOOL bPolyDrawn)
{
	int idx=pstyle->LineIdx();

	if((bNoVectors || idx==SEG_NO_LINEIDX) && bPolyDrawn) return TRUE;

	if((iGradIdx && pstyle->GradIdx()==iGradIdx) || pstyle->LineColor()==pSV->FloorColor()) {
		return (bNoVectors || idx==SEG_NO_LINEIDX)?TRUE:(idx==0);
	}
	//Color doesn't match. Must have !bPolyDrawn OR !(bNoVectors || idx==SEG_NO_LINEIDX)
	return FALSE;
}

static apfcn_v PlotLrud(int typ,BOOL bPoly)
{
	static double x0,y0,rawx0,rawy0;
	static int end_id0;
	int end_id;
	UINT bClip,ptCnt;
	double x,y;

	POINT pt[8];

	//typ==0 - MoveTo or invisible vector
	//typ==1 - Visible vector

	if(!(end_id=pPLT->end_id)) {
		ASSERT(!iGradIdx);
		bClipped=(UINT)-1;
		end_id0=0;
		bLastLrud=FALSE;
		return;
	}

	//Let's work in page coordinates and survey units 
	x=TransAx(pPLT->xyz)-xoff;
	y=TransAy(pPLT->xyz)+yoff;

	if(iGradIdx) {
		//We are accessing dbNTP randomly -- initialize FROM station data --
		VERIFY(!dbNTP.Prev());
		end_id0=pPLT->end_id;
		ASSERT(end_id0);
		rawx0=pPLT->xyz[0];
		rawy0=pPLT->xyz[1];
		zLastPt=pPLT->xyz[2];
		x0=TransAx(pPLT->xyz)-xoff;
		y0=TransAy(pPLT->xyz)+yoff;
		VERIFY(!dbNTP.Next());
		bLastLrud=FALSE;

		if(!end_id0) zLastPt=pPLT->xyz[2];
		COLORREF clr=GetPltGradientColor(iGradIdx-1);
		if(clr!=clrGradient) {
			HBRUSH hBrush=(HBRUSH)::CreateSolidBrush(clrGradient=clr);
			ASSERT(hBrush);
			VERIFY(hBrush=(HBRUSH)::SelectObject(pvDC->m_hDC,hBrush));
			::DeleteObject(hBrush);
		}
	}
	else bClip=OutCode(x*xscale,-y*xscale);

	//The vector is entirely outside if (but not only if) bClip&bClipped != 0.
	if(iGradIdx || (typ && (bClip&bClipped)==0)) {
		
		BOOL bVertical=(end_id0==0) || (fabs(rawx0-pPLT->xyz[0])<0.2 && fabs(rawy0-pPLT->xyz[1])<0.2);
		BOOL bLrudVector=FALSE;
		double vec_az;

		if(bVertical) {
			vec_az=LRUD_BLANK;
			//vec_az=fView*(PI/180.0);
			//if(vec_az<0.0) vec_az+=2*PI;
		}
		else {
			if(bProfile) {
				vec_az=fmod(atan2(pPLT->xyz[0]-rawx0,pPLT->xyz[1]-rawy0)-fView*(PI/180.0),2*PI);
			}
			else vec_az=atan2(x-x0,y-y0);
			if(vec_az<0.0) vec_az+=2*PI;
		}


		bLrudVector=bPoly && end_id0 && (!bVertical || bProfile);

		if(end_id0 && !bLastLrud && getRayPoints(x0,y0,end_id0,bPoly)) {
			if(!bPoly) PlotRays();
			else if(rayCnt>1 || bProfile) {
				//Draw FROM station polygon --
				PlotRayPolygon();
			}
		}
		else bLrudVector=bLastLrud && bLrudVector;

		if(bLrudVector)	ptCnt=GetPointsFromRays(pt,vec_az);

		//We are now through with the from station rays --
		if(!bVertical && (vec_az+=PI)>=2*PI) vec_az-=2*PI;
		bLastLrud=FALSE;

		if(getRayPoints(x,y,end_id,bPoly)) {
			bLastLrud=TRUE;

			if(!bPoly) PlotRays();
			else if(rayCnt>1 || bProfile) {
				//Draw TO station polygon --
				PlotRayPolygon();
			}

			if(bLrudVector) {
				ptCnt+=GetPointsFromRays(pt+ptCnt,vec_az);
				if((ptCnt=GetConvexHull(pt,ptCnt))>2) {
					//Draw polygon around vector --
					::Polygon(pvDC->m_hDC,pt,ptCnt);
				}
			}
		}
		else bLrudVector=FALSE;

		if(bPoly && (!bVertical || bProfile)) {
			//Let's determine if a vector needs to be drawn in the floor color.
			//The two requirements are 1) lineidx=0 or SEG_NO_LINEIDX, or bNoVectors,
			//and 2) the color or gradient assigned to this segment is the same as
			//what's being used for the floors.
			if(VectorNeeded(
					(mapStr_id && mapStr_id==pPLT->str_id)?pSV->TravStyle():pSV->SegNode(pPLT->seg_id)->Style()
					,bLrudVector)) {
				ASSERT(end_id0 && pPLT->vec_id && pPLT->end_id);
				PlotLrudVector(x0,y0,x,y);
			}
		}
	}
	else bLastLrud=FALSE;

//_exit:
	if(!iGradIdx) {
		rawx0=pPLT->xyz[0];
		rawy0=pPLT->xyz[1];
		end_id0=end_id;
		bClipped=bClip;
		x0=x;
		y0=y;
		zLastPt=pPLT->xyz[2];
	}
}

static int FAR PASCAL PltSeqFcn(int i)
{
	double d=pPltZ[numPltRec]-pPltZ[i];
	return d>0.0?1:(d<0.0?-1:0);
}

static BOOL AllocPltRec()
{
	DWORD irec=(1+pNTN->plt_id2-pNTN->plt_id1);
	double x,y;
	int end_id0=0;
	UINT bClip;

	if(!(pPltRec=(DWORD *)malloc((sizeof(DWORD)+sizeof(double))*irec))) return FALSE;
	pPltZ=(double *)(pPltRec+irec);
	numSeqRec=0;
	trx_Bininit(pPltRec,&numSeqRec,PltSeqFcn);

	for(irec=pNTN->plt_id1;irec<=pNTN->plt_id2;irec++) {
		if(dbNTP.Go(irec)) break;
		if(pPLT->end_id && end_id0) {
			x=TransAx(pPLT->xyz)-xoff;
			y=TransAy(pPLT->xyz)+yoff;
			bClip=OutCode(x*xscale,-y*xscale);
			//The vector is entirely outside if (but not only if) bClip&bClipped != 0.
			if(pPLT->vec_id && pSV->SegNode(pPLT->seg_id)->IsVisible() && (bClip&bClipped)==0) {
				numPltRec=irec-pNTN->plt_id1;
				pPltZ[numPltRec]=bProfile?TransAz(pPLT->xyz):(pPLT->xyz[2]+zLastPt)/2.0;
				trx_Binsert(numPltRec,TRX_DUP_IGNORE);
			}
			bClipped=bClip;
			zLastPt=pPLT->xyz[2];
		}
		end_id0=pPLT->end_id;
	}
	return TRUE;
}

void PlotLrudPolys()
{
	//  flags --
	//  LRUD_SOLID:   Colored brush and colored pen
	//  LRUD_OUTLINE: Use NULL brush and colored pen
	//  0:            Use background brush and NULL pen

    UINT irec;
	HPEN hPenOld=(HPEN)::GetStockObject(NULL_PEN);
    HBRUSH hBrushOld;

	if(pMF->bUseColors) {
		if((iGradIdx=pSV->FloorGradIdx()) && !AllocPltRec()) iGradIdx=0;
		VERIFY(hBrushOld=(HBRUSH)::CreateSolidBrush(clrGradient=pSV->FloorColor()));
	}
	else {
		iGradIdx=0;
		hBrushOld=(HBRUSH)::GetStockObject(BLACK_BRUSH);
	}

	VERIFY(hPenOld=(HPEN)::SelectObject(pvDC->m_hDC,hPenOld));
	VERIFY(hBrushOld=(HBRUSH)::SelectObject(pvDC->m_hDC,hBrushOld));
	bLastLrud=FALSE;

	if(iGradIdx) {
		for(irec=0;irec<numSeqRec;irec++) {
			if(dbNTP.Go(pPltRec[irec]+pNTN->plt_id1)) break;
			PlotLrud(TRUE,TRUE);
		}
		free(pPltRec); 
	}
	else {
		for(irec=pNTN->plt_id1;irec<=pNTN->plt_id2;irec++) {
			if(dbNTP.Go(irec)) break;
			PlotLrud(pPLT->vec_id && pSV->SegNode(pPLT->seg_id)->IsVisible(),TRUE);
		}
	}
	
	::DeleteObject(::SelectObject(pvDC->m_hDC,hBrushOld));
	::DeleteObject(::SelectObject(pvDC->m_hDC,hPenOld));
}

//============================================================================

void PlotLrudTicks(BOOL bTicksOnly)
{
    UINT irec=pNTN->plt_id1;
    
	if(dbNTP.Go(irec)) return;

	iGradIdx=0;

	HPEN hPenOld;

	styletyp style=*pSV->OutlineStyle();

	if(style.IsNoLines()) // || style.LineIdx()==CSegView::PS_HEAVY_IDX)
		style.SetLineIdx(CSegView::PS_SOLID_IDX);
	if(!pMF->bUseColors) style.SetLineColor(bTicksOnly?RGB_BLACK:RGB_WHITE);

	VERIFY(hPenOld=GetStylePen(&style));

	VERIFY(hPenOld=(HPEN)::SelectObject(pvDC->m_hDC,hPenOld));

	for(;irec<=pNTN->plt_id2;irec++) {

		PlotLrud(pPLT->vec_id && pSV->SegNode(pPLT->seg_id)->IsVisible(),FALSE);
		  
		if(dbNTP.Next()) break;
	}
	::DeleteObject(::SelectObject(pvDC->m_hDC,hPenOld));
}
