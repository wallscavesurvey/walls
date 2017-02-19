#include "stdafx.h"
#include "ShpLayer.h"
#include "QuadTree.h"

static double PointSegmentDistanceSquared( double px, double py,
                                    double p1x, double p1y,
                                    double p2x, double p2y,
                                    //double& t,
                                    double& qx, double& qy)
{
    static const double kMinSegmentLenSquared = 0.00000001;  // adjust to suit.  If you use float, you'll probably want something like 0.000001f
    static const double kEpsilon = 1.0E-14;  // adjust to suit.  If you use floats, you'll probably want something like 1E-7f
	double t;
    double dx = p2x - p1x;
    double dy = p2y - p1y;
    double dp1x = px - p1x;
    double dp1y = py - p1y;
    const double segLenSquared = (dx * dx) + (dy * dy);
    if (segLenSquared >= -kMinSegmentLenSquared && segLenSquared <= kMinSegmentLenSquared)
    {
        // segment is a point.
        qx = p1x;
        qy = p1y;
        t = 0.0;
        return ((dp1x * dp1x) + (dp1y * dp1y));
    }
    else
    {
        // Project a line from p to the segment [p1,p2].  By considering the line
        // extending the segment, parameterized as p1 + (t * (p2 - p1)),
        // we find projection of point p onto the line. 
        // It falls where t = [(p - p1) . (p2 - p1)] / |p2 - p1|^2
        t = ((dp1x * dx) + (dp1y * dy)) / segLenSquared;
        if (t < kEpsilon)
        {
            // intersects at or to the "left" of first segment vertex (p1x, p1y).  If t is approximately 0.0, then
            // intersection is at p1.  If t is less than that, then there is no intersection (i.e. p is not within
            // the 'bounds' of the segment)
            if (t > -kEpsilon)
            {
                // intersects at 1st segment vertex
                t = 0.0;
            }
            // set our 'intersection' point to p1.
            qx = p1x;
            qy = p1y;
            // Note: If you wanted the ACTUAL intersection point of where the projected lines would intersect if
            // we were doing PointLineDistanceSquared, then qx would be (p1x + (t * dx)) and qy would be (p1y + (t * dy)).
        }
        else if (t > (1.0 - kEpsilon))
        {
            // intersects at or to the "right" of second segment vertex (p2x, p2y).  If t is approximately 1.0, then
            // intersection is at p2.  If t is greater than that, then there is no intersection (i.e. p is not within
            // the 'bounds' of the segment)
            if (t < (1.0 + kEpsilon))
            {
                // intersects at 2nd segment vertex
                t = 1.0;
            }
            // set our 'intersection' point to p2.
            qx = p2x;
            qy = p2y;
            // Note: If you wanted the ACTUAL intersection point of where the projected lines would intersect if
            // we were doing PointLineDistanceSquared, then qx would be (p1x + (t * dx)) and qy would be (p1y + (t * dy)).
        }
        else
        {
            // The projection of the point to the point on the segment that is perpendicular succeeded and the point
            // is 'within' the bounds of the segment.  Set the intersection point as that projected point.
            qx = p1x + (t * dx);
            qy = p1y + (t * dy);
        }
        // return the squared distance from p to the intersection point.  Note that we return the squared distance
        // as an optimization because many times you just need to compare relative distances and the squared values
        // works fine for that.  If you want the ACTUAL distance, just take the square root of this value.
        double dpqx = px - qx;
        double dpqy = py - qy;
        return ((dpqx * dpqx) + (dpqy * dpqy));
    }
}

static bool pnpoly(int npol, CFltPoint *point, double x, double y)
{
    int i, j, c = 0;
    for (i = 0, j = npol-1; i < npol; j = i++) {
    if ((((point[i].y<=y) && (y<point[j].y)) ||
            ((point[j].y<=y) && (y<point[i].y))) &&
        (x < (point[j].x - point[i].x) * (y - point[i].y) / (point[j].y - point[i].y) + point[i].x))

        c = !c;
    }
    return c>0;
}

static double DistFromPolySquared(int ptCount, CFltPoint *pt, double x, double y,double &qx,double &qy)
{
	ASSERT(ptCount>=3);
	double d,qxmin,qymin,dmin=DBL_MAX;
    for (int i = 0; i<ptCount-1; i++,pt++) {
		d=PointSegmentDistanceSquared(x, y, pt->x, pt->y,pt[1].x,pt[1].y, qxmin, qymin);
		if(d<dmin) {
			dmin=d;
			qx=qxmin;
			qy=qymin;
		}
   }
   return dmin;
}

DWORD CShpLayer::ShpPolyTest(const CFltPoint &fpt,DWORD recNo /*=0*/,double *pDist /*=NULL*/)
{

	ASSERT(!pDist || !*pDist);
	if(ShpType()!=SHP_POLYGON || !m_extent.IsPtInside(fpt)) return 0;

	SHP_POLY_DATA *ppoly;
	if(!m_nNumRecs || !(ppoly=Init_ppoly())) return 0;
	if(!recNo && m_pQT) {
		ASSERT(m_bQTusing);
		return m_pQT->GetPolyRecContainingPt(fpt);
	}

	ASSERT(m_ext.size()==m_nNumRecs);

	CFileMap &cf=*m_pdbfile->pfShp;

	DWORD lastRec;

	if(recNo) {
		lastRec=recNo;
		recNo--;
	    ppoly+=recNo;
	}
	else lastRec=m_nNumRecs;

	try {
		for(;recNo<lastRec;recNo++,ppoly++) {

			if(ppoly->IsDeleted())	continue;

			int *parts=(int *)m_pdbfile->pbuf;
			if(!parts || ppoly->Len()>m_pdbfile->pbuflen) {
				parts=(int *)Realloc_pbuf(ppoly->Len());
				if(!parts)
					throw 0;
			}
			cf.Seek(ppoly->off,CFile::begin);
			if(cf.Read(parts,ppoly->Len())!=ppoly->Len())
				throw 0; //error
			int iLastPart=*parts++; //numParts
			int numPoints=*parts++; //total point count
			ASSERT(parts[0]==0);

			CFltPoint *pt=(CFltPoint *)&parts[iLastPart--];

			if(m_bConverted)
					ConvertPointsFromOrg((CFltPoint *)pt,numPoints);


			if(!m_ext[recNo].IsPtInside(fpt))
				goto _next;

			bool bInside=false;
			for(int iPart=0;iPart<=iLastPart;iPart++) {
				int i=parts[iPart];
				int count=((iPart<iLastPart)?parts[iPart+1]:numPoints)-i;
				if(pnpoly(count, pt+i, fpt.x, fpt.y)) {
					bInside=!bInside;
					if(!bInside) break;
				}
			}
			if(bInside) return recNo+1;
_next:
			if(pDist) {
				ASSERT(!m_iZone);
			    int iZone=geo_GetZone(fpt.y,fpt.x);
				CFltPoint ptUTM(fpt);
				ConvertPointsTo(&ptUTM,m_iNad,iZone,1);
				ConvertPointsTo(pt,m_iNad,iZone,numPoints);
			    double qx,qy,qxmin,qymin,distMin=DBL_MAX;
				for(int iPart=0;iPart<=iLastPart;iPart++) {
					int i=parts[iPart];
					int ptCount=((iPart<iLastPart)?parts[iPart+1]:numPoints)-i;
					double d=DistFromPolySquared(ptCount, pt+i, ptUTM.x, ptUTM.y, qx, qy);
					if(d<distMin) {
						distMin=d;
						qxmin=qx;
						qymin=qy;
					}
				}
				if(distMin<DBL_MAX) {
					*pDist=(distMin>0.0)?sqrt(distMin):0.0;
					return -1;
				}
			}
		}
	}
	catch(...) {
		ASSERT(0);
	}

	return 0;
}
