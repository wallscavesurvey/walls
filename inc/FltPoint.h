//to be included in ShpLayer.h and PolygonClip.h --
#ifndef _FLTPOINT_H
#define _FLTPOINT_H

#ifndef _VECTOR_
#include <vector>
#endif

struct CFltPoint {
	CFltPoint() : x(0), y(0) {}
	CFltPoint(const CFltPoint &pt) : x(pt.x), y(pt.y) {}
	CFltPoint(double xp, double yp) : x(xp), y(yp) {}
	bool operator==(const CFltPoint &rhs) const
	{
		return (x == rhs.x) && (y == rhs.y);
	}

	double x;
	double y;
};

typedef std::vector<double> VEC_FLT;
typedef VEC_FLT::iterator it_flt;
typedef std::vector<CFltPoint> VEC_FLTPOINT;
typedef VEC_FLTPOINT::iterator it_fltpt;

union CFltRect {
	//CFltRect(double fl,double ft,double fr,double fb) : l(fl),t(ft),r(fr),b(fb) {} //FAILS!!!???
	CFltRect() : l(0), t(0), r(0), b(0) {}
	CFltRect(const CFltRect &rect) : tl(rect.tl), br(rect.br) {}
	CFltRect(double fl, double ft, double fr, double fb) : tl(fl, ft), br(fr, fb) {}
	CFltRect(const CFltPoint &ftl, const CFltPoint &fbr) : tl(ftl.x, ftl.y), br(fbr.x, fbr.y) {}
	CFltPoint Center() const { return CFltPoint((r + l) / 2, (b + t) / 2); }
	CFltPoint Size() const { return CFltPoint(r - l, fabs(t - b)); }

	bool operator==(const CFltRect &rhs) const
	{
		return (l == rhs.l) && (r == rhs.r) && (t == rhs.t) && (b == rhs.b);
	}

	bool IsPtInside(const CFltPoint &pt) const
	{
		//assumes m_bTrans!
		return pt.x >= l && pt.x<r && pt.y <= t && pt.y>b;
	}
	bool IsRectOutside(const CFltRect &rect) const
	{
		//assumes m_bTrans!
		return rect.l >= r || rect.r<l || rect.t <= b || rect.b>t;
	}
	bool IsRectInside(const CFltRect &rect) const
	{
		//assumes m_bTrans!
		return IsPtInside(rect.tl) && rect.r >= l && rect.r <= r && rect.b <= t && rect.b >= b;
	}

	bool IntersectWith(const CFltRect &rect)
	{
		bool bRet = false;
		if (l<rect.r && r>rect.l) {
			if (rect.t > rect.b && t > b) {
				//Georeferenced case --
				if (b<rect.t && t>rect.b) {
					if (t > rect.t) t = rect.t;
					if (b < rect.b) b = rect.b;
					bRet = true;
				}
			}
			else if (rect.b > rect.t && b > t) {
				if (b > rect.t && t < rect.b) {
					if (t < rect.t) t = rect.t;
					if (b > rect.b) b = rect.b;
					bRet = true;
				}
			}
			if (bRet) {
				if (l < rect.l) l = rect.l;
				if (r > rect.r) r = rect.r;
			}
		}
		if (!bRet) { r = l; b = t; }
		return bRet;
	}

	bool ContainsRect(const CFltRect &rect) const
	{
		//Allow single point extents --
		if (rect.l <= rect.r && rect.l >= l && rect.r <= r) {
			if (t >= b) {
				//Georeferenced case --
				if (rect.t >= rect.b && rect.t <= t && rect.b >= b)
					return true;
			}
			else if (rect.t <= rect.b && rect.t >= t && rect.b <= b)
				return true;
		}
		return false;
	}

	bool IsValid() { return l <= r && b <= t; }

	double Width() const { return r - l; }
	double Height() const { return fabs(t - b); }
	struct {
		double l;
		double t;
		double r;
		double b;
	};
	struct {
		CFltPoint tl;
		CFltPoint br;
	};
};

typedef std::vector<CFltRect> VEC_FLTRECT;
typedef VEC_FLTRECT::iterator it_rect;

#endif