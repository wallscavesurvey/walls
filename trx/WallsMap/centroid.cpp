#include "stdafx.h"
#include <math.h>

/*
 * ANSI C code from the article
 * "Centroid of a Polygon"
 * by Gerard Bashein and Paul R. Detmer,
	(gb@locke.hs.washington.edu, pdetmer@u.washington.edu)
 * in "Graphics Gems IV", Academic Press, 1994
 */

// I designed this with GDI+ in mind. However, this particular code doesn't
// use GDI+ at all, only some of it's variable types.
// These definitions are substitutes for those of GDI+.

/*********************************************************************
polyCentroid: Calculates the centroid (xCentroid, yCentroid) and area
of a polygon, given its vertices (x[0], y[0]) ... (x[n-1], y[n-1]). It
is assumed that the contour is closed, i.e., that the vertex following
(x[n-1], y[n-1]) is (x[0], y[0]).  The algebraic sign of the area is
positive for counterclockwise ordering of vertices in x-y plane;
otherwise negative.

Returned values:  0 for normal execution;  1 if the polygon is
degenerate (number of vertices < 3);  and 2 if area = 0 (and the
centroid is undefined).
**********************************************************************/
#if 0
int polyCentroid(double x[], double y[], int n,
		 double *xCentroid, double *yCentroid, double *area)
{
     register int i, j;
     double ai, atmp = 0, xtmp = 0, ytmp = 0;
     if (n < 3) return 1;
     for (i = n-1, j = 0; j < n; i = j, j++)
	  {
	  ai = x[i] * y[j] - x[j] * y[i];
	  atmp += ai;
	  xtmp += (x[j] + x[i]) * ai;
	  ytmp += (y[j] + y[i]) * ai;
	  }
     *area = atmp / 2;
     if (atmp != 0)
	  {
	  *xCentroid =	xtmp / (3 * atmp);
	  *yCentroid =	ytmp / (3 * atmp);
	  return 0;
	  }
     return 2;
}
#endif

BOOL PolyCentroid(LPPOINT pCentroid, LPPOINT points, LPINT pnPoints,int nPolys)
{
	double sumAreas=0,sumProdX=0,sumProdY=0;
	while(nPolys--) {
		int npts=*pnPoints++;
		if(npts>2) {
			double atmp=0,xtmp=0,ytmp=0;
			double Xi=points[npts-1].x,Yi=points[npts-1].y;

			for (int j = 0; j < npts; j++)
			{
				double Xj=points[j].x, Yj=points[j].y;
				double ai = Xi*Yj - Xj*Yi;
				atmp += ai;
				xtmp += (Xj + Xi) * ai;
				ytmp += (Yj + Yi) * ai;
				Xi=Xj; Yi=Yj;
			}
			Xi=xtmp/(3*atmp);
			Yi=ytmp/(3*atmp);
			atmp=fabs(atmp);
			if(atmp>=2.0) {
				sumProdX+=Xi*atmp;
				sumProdY+=Yi*atmp;
				sumAreas+=atmp;
			}
		}
		points+=npts;
	}
	if(!sumAreas) return FALSE;
	pCentroid->x=(LONG)(sumProdX/sumAreas);
	pCentroid->y=(LONG)(sumProdY/sumAreas);
	return TRUE;
}

#if 0
BOOL PolyCentroidF(CD2DPointF &pCentroid,CD2DPointF *points, LPINT pnPoints,int nPolys)
{
	double sumAreas=0,sumProdX=0,sumProdY=0;
	while(nPolys--) {
		int npts=*pnPoints++;
		if(npts>2) {
			double atmp=0,xtmp=0,ytmp=0;
			double Xi=points[npts-1].x,Yi=points[npts-1].y;

			for (int j = 0; j < npts; j++)
			{
				double Xj=points[j].x, Yj=points[j].y;
				double ai = Xi*Yj - Xj*Yi;
				atmp += ai;
				xtmp += (Xj + Xi) * ai;
				ytmp += (Yj + Yi) * ai;
				Xi=Xj; Yi=Yj;
			}
			Xi=xtmp/(3*atmp);
			Yi=ytmp/(3*atmp);
			atmp=fabs(atmp);
			if(atmp>=2.0) {
				sumProdX+=Xi*atmp;
				sumProdY+=Yi*atmp;
				sumAreas+=atmp;
			}
		}
		points+=npts;
	}
	if(!sumAreas) return FALSE;
	pCentroid.x=(float)(sumProdX/sumAreas);
	pCentroid.y=(float)(sumProdY/sumAreas);
	return TRUE;
}
#endif