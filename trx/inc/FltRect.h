#ifndef _FLTRECT_H
#define _FLTRECT_H

struct CFltPoint {
	CFltPoint() : x(0),y(0) {}
	CFltPoint(const CFltPoint &pt) : x(pt.x),y(pt.y) {}
	CFltPoint(double xp,double yp) : x(xp),y(yp) {}
	double x;
	double y;
};

union CFltRect {
	//CFltRect(double fl,double ft,double fr,double fb) : l(fl),t(ft),r(fr),b(fb) {} //FAILS!!!???
	CFltRect() {}
	CFltRect(const CFltRect &rect) : tl(rect.tl),br(rect.br) {}
	CFltRect(double fl,double ft,double fr,double fb) : tl(fl,ft),br(fr,fb) {}
	CFltRect(const CFltPoint &ftl,const CFltPoint &fbr) : tl(ftl.x,ftl.y),br(fbr.x,fbr.y) {}
	CFltPoint Center() const {return CFltPoint((r+l)/2,(b+t)/2);}
	CFltPoint Size() const {return CFltPoint(r-l,t-b);}
	BOOL IsPtInside(const CFltPoint &pt) const
	{
		return pt.x>=l && pt.x<r && pt.y<=t && pt.y>b;
	}
	BOOL IsRectOutside(const CFltRect &rect) const
	{
		return rect.l>=r || rect.r<l || rect.t<=b || rect.b>t;
	}
	BOOL IsRectInside(const CFltRect &rect) const
	{
		return IsPtInside(rect.tl) && rect.r>=l && rect.r<=r && rect.b<=t && rect.b>=b;
	}
	double Width() const {return r-l;}
	double Height() const {return t-b;}
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
#endif