#include "stdafx.h"           
#include "walls.h"
#include "ntfile.h"
#include <trxfile.h>
#include "compview.h"
#include "plotview.h"
#include "segview.h"
#include "compile.h"

//Functions in HULL2D.CPP --
UINT GetConvexHull(POINT *pt, UINT ptCnt);
UINT ElimDups(POINT *pt, UINT ptCnt);

//Function in plotlrud.cpp --
int getRayPoints(double x, double y, int id, BOOL bPoly);

static BOOL bLastLrud;
static DWORD *pPltRec;
static double *pPltZ;
static DWORD numSeqRec, numPltRec;
static LRUDPOLY_CB pOutPoly;

static UINT ElimDupsF(CFltPoint *pt, UINT ptCnt)
{
	for (UINT n = 0; n < ptCnt; n++) {
		vPnt[n].x = (long)(100.0*pt[n].x);
		vPnt[n].y = (long)(100.0*pt[n].y);
	}
	ptCnt = ElimDups(&vPnt[0], ptCnt);
	if (ptCnt > 2) {
		for (UINT n = 0; n < ptCnt; n++) {
			pt[n].x = vPnt[n].x / 100.0;
			pt[n].y = vPnt[n].y / 100.0;
		}
	}
	return ptCnt;
}

static UINT GetConvexHullF(SHP_TYP_LRUDPOLY &poly)
{
	POINT lpt[8];
	UINT ptCnt = poly.nPts;
	for (UINT n = 0; n < ptCnt; n++) {
		lpt[n].x = (long)(100.0*poly.fpt[n].x);
		lpt[n].y = (long)(100.0*poly.fpt[n].y);
	}
	ptCnt = GetConvexHull(lpt, ptCnt);
	if (ptCnt > 2) {
		for (UINT n = 0; n < ptCnt; n++) {
			poly.fpt[n].x = lpt[n].x / 100.0;
			poly.fpt[n].y = lpt[n].y / 100.0;
		}
		poly.nPts = ptCnt;
	}
	return ptCnt;
}

static apfcn_v get_rotated_pt(CFltPoint *pt, RAYPOINT *rp, double sa)
{
	RAYPOINT *rp0 = &vRayPnt[rayCnt];
	double x = rp->x - rp0->x;
	double y = rp->y - rp0->y;
	double ca = cos(sa);

	sa = sin(sa);
	*pt = CFltPoint(rp0->x + x * ca + y * sa, rp0->y + y * ca - x * sa);
}

static void outPoly(SHP_TYP_LRUDPOLY &poly)
{
	if (bUseMag) {
		for (int i = 0; i < poly.nPts; i++) {
			CMainFrame::ConvertUTM2LL(&poly.fpt[i].x); //same as below
			/*
			MD.east=poly.fpt[i].x;
			MD.north=poly.fpt[i].y;
			if(MF(MAG_CVTUTM2LL3)) {
				//returned MAG_ERR_CVTUTM2LL
				ASSERT(0);
				poly.fpt[i].x=poly.fpt[i].y=0.0;
			}
			else {
				poly.fpt[i].x=MD.lon.fdeg*MD.lon.isgn;
				poly.fpt[i].y=MD.lat.fdeg*MD.lat.isgn;
			}
			*/
		}
	}
	pOutPoly(&poly);
}

static BOOL OutRayPolygon(double z)
{
	SHP_TYP_LRUDPOLY poly;
	RAYPOINT *rp = &vRayPnt[0];
	int r = rayCnt;
	int ptCnt = 0;
	BOOL bCorner = FALSE;
	double x, y;
	double lastaz = vRayPnt[rayCnt - 1].az - 2 * PI;

	while (r) {
		if (!bCorner && (rp->az - lastaz) > (1.01*PI)) {
			bCorner = TRUE;
			x = vRayPnt[rayCnt].x;
			y = vRayPnt[rayCnt].y;
		}
		else {
			lastaz = rp->az;
			x = rp->x;
			y = rp->y;
			rp++;
			r--;
		}
		poly.fpt[ptCnt++] = CFltPoint(x, y);
		if (ptCnt == MAX_LRUDPOLY_PTS) break;
	}
	if ((ptCnt = ElimDupsF(poly.fpt, ptCnt)) <= 2) return FALSE;
	poly.nPts = ptCnt;
	poly.elev = z;
	outPoly(poly);
	pExpShp->IncPolyCnt();
	return TRUE;
}

static apfcn_v GetPointsFromRays(SHP_TYP_LRUDPOLY &poly, double vec_az)
{
	UINT i, lasti;
	double nextaz, lastaz;
	CFltPoint *pt = &poly.fpt[poly.nPts];

	UINT ptCnt = 0;

	ASSERT(vec_az != (double)LRUD_BLANK);
	if (rayCnt == 1) {
		i = 0;
		lasti = 1;
	}
	else {
		lastaz = vRayPnt[lasti = rayCnt - 1].az - 2 * PI;
		for (i = 0; i < rayCnt; i++) {
			ASSERT(vRayPnt[i].az >= lastaz);
			if (vec_az <= vRayPnt[i].az) break;
			lastaz = vRayPnt[lasti = i].az;
		}
		if (i < rayCnt) nextaz = vRayPnt[i].az;
		else nextaz = vRayPnt[i = 0].az + 2 * PI;
		if (rayCnt == 2 && fabs(nextaz - lastaz - PI) < 0.01) {
			//Use Enlargement factor --
			nextaz = (pSV->LrudEnlarge() / 10.0)*(vec_az - lastaz - PI2);
			//nextaz is amount to rotate points CW about station
			if (fabs(nextaz) >= (2.0 / 180.0)*PI) {
				get_rotated_pt(&pt[0], &vRayPnt[lasti], nextaz);
				get_rotated_pt(&pt[1], &vRayPnt[i], nextaz);
				ptCnt = 2;
			}
		}
		else {
			if (vec_az - lastaz > PI) lasti = rayCnt;
			else if (nextaz - vec_az > PI) i = rayCnt;
		}
	}

	pt[ptCnt++] = CFltPoint(vRayPnt[lasti].x, vRayPnt[lasti].y);
	if (i != lasti) {
		pt[ptCnt++] = CFltPoint(vRayPnt[i].x, vRayPnt[i].y);
	}
	poly.nPts += ptCnt;
}

static apfcn_v OutVecPolygons()
{
	static double x0, y0, z0, rawx0, rawy0;
	static int end_id0;
	int end_id;
	double x, y, z;
	SHP_TYP_LRUDPOLY poly;

	//Visible vector

	end_id = pPLT->end_id;
	ASSERT(end_id);

	//Let's work in survey units 
	x = pPLT->xyz[0];
	y = pPLT->xyz[1];
	z = pPLT->xyz[2];

	//We are accessing dbNTP randomly -- initialize FROM station data --
	VERIFY(!dbNTP.Prev());
	end_id0 = pPLT->end_id;
	ASSERT(end_id0);
	x0 = pPLT->xyz[0];
	y0 = pPLT->xyz[1];
	z0 = pPLT->xyz[2];
	VERIFY(!dbNTP.Next());
	bLastLrud = FALSE;

	BOOL bVertical = (end_id0 == 0) || (fabs(x0 - pPLT->xyz[0]) < 0.2 && fabs(y0 - pPLT->xyz[1]) < 0.2);
	double vec_az;

	if (bVertical) {
		vec_az = LRUD_BLANK;
		//vec_az=fView*(PI/180.0);
		//if(vec_az<0.0) vec_az+=2*PI;
	}
	else {
		vec_az = atan2(x - x0, y - y0);
		if (vec_az < 0.0) vec_az += 2 * PI;
	}

	BOOL bLrudVector = end_id0 && !bVertical;

	if (end_id0 && !bLastLrud && getRayPoints(x0, y0, end_id0, TRUE)) {
		if (rayCnt > 1) {
			//Draw FROM station polygon --
			OutRayPolygon(z0);
		}
	}
	else bLrudVector = bLastLrud && bLrudVector;

	poly.nPts = 0;
	if (bLrudVector) GetPointsFromRays(poly, vec_az);

	//We are now through with the from station rays --
	if (!bVertical && (vec_az += PI) >= 2 * PI) vec_az -= 2 * PI;
	bLastLrud = FALSE;

	if (getRayPoints(x, y, end_id, TRUE)) {
		bLastLrud = TRUE;

		if (rayCnt > 1) {
			//Draw TO station polygon --
			OutRayPolygon(z);
		}

		if (bLrudVector) {
			GetPointsFromRays(poly, vec_az);
			if (GetConvexHullF(poly) > 2) {
				//Draw polygon around vector --
				poly.elev = (z + z0) / 2.0;
				outPoly(poly);
				pExpShp->IncPolyCnt();
			}
		}
	}
}

static int FAR PASCAL PltSeqFcn(int i)
{
	double d = pPltZ[numPltRec] - pPltZ[i];
	return d > 0.0 ? 1 : (d < 0.0 ? -1 : 0);
}

BOOL AllocPltRecLrud()
{
	DWORD irec = (1 + pNTN->plt_id2 - pNTN->plt_id1);
	int end_id0 = 0;

	if (!(pPltRec = (DWORD *)malloc((sizeof(DWORD) + sizeof(double))*irec))) return FALSE;
	pPltZ = (double *)(pPltRec + irec);
	numSeqRec = 0;
	trx_Bininit(pPltRec, &numSeqRec, PltSeqFcn);

	for (irec = pNTN->plt_id1; irec <= pNTN->plt_id2; irec++) {
		if (dbNTP.Go(irec)) break;
		if (pPLT->end_id) {
			if (end_id0 && pPLT->vec_id && pSV->SegNode(pPLT->seg_id)->IsVisible()) {
				numPltRec = irec - pNTN->plt_id1;
				pPltZ[numPltRec] = (pPLT->xyz[2] + zLastPt) / 2.0;
				trx_Binsert(numPltRec, TRX_DUP_IGNORE);
			}
			zLastPt = pPLT->xyz[2];
		}
		end_id0 = pPLT->end_id;
	}
	return TRUE;
}

int WriteLrudPolygons(LRUDPOLY_CB pOP)
{
	int e = 0;
	if (!AllocPltRecLrud()) return SHP_ERR_NOMEM;
	/*
	if(!(rayPnt=(RAYPOINT *)malloc((MAX_RAYPOINTS+2)*sizeof(RAYPOINT)))) {
		free(pPltRec);
		return SHP_ERR_NOMEM;
	}
	*/
	BOOL bProfileSave = bProfile;
	bProfile = FALSE;
	double fViewSave = fView;
	fView = 0.0;
	pOutPoly = pOP;
	ixSEG.UseTree(NTA_LRUDTREE);
	for (UINT irec = 0; irec < numSeqRec; irec++) {
		if (dbNTP.Go(pPltRec[irec] + pNTN->plt_id1)) {
			e = SHP_ERR_VERSION; //shouldn't happen
			break;
		}
		OutVecPolygons();
	}
	ixSEG.UseTree(NTA_SEGTREE);
	vRayPnt.clear();
	vPnt.clear();
	free(pPltRec);
	fView = fViewSave;
	bProfile = bProfileSave;
	return e;
}
