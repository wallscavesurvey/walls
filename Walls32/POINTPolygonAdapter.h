#pragma once

#include "Polygon2d.h"
#include "wall_shp.h"
#include "windef.h"

class POINTPolygonAdapter :
    public Polygon2d<long>
{
public:
    POINTPolygonAdapter(int nPoints, POINT *pPoints) : nPoints(nPoints), pPoints(pPoints) {}
    inline int numPoints() { return nPoints; }
    inline Polygon2dPoint<long> getPoint(int index) { 
        POINT* pPoint = pPoints + index;
        return Polygon2dPoint<long> { pPoint->x, pPoint->y };
    }
private:
    int nPoints;
    POINT *pPoints;
};

