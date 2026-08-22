#pragma once

#include <type_traits>

template<typename T>
struct Polygon2dPoint {
	T x, y;
};

template<typename T>
struct Polygon2dBounds {
	T x0, x1, y0, y1;
};

enum FillRule { nonzero, evenodd };

template<typename T>
class Polygon2d
{
public:
	virtual int numPoints() = 0;
	virtual Polygon2dPoint<T> getPoint(int index) = 0;

	bool isClockwise();
	Polygon2dBounds<T> getBounds();

	bool contains(Polygon2dPoint<T> point, FillRule rule = nonzero);
};


template <typename T>
bool Polygon2d<T>::isClockwise()
{
	int nPoints = numPoints();
	if (nPoints < 3) return true;

	int minYIndex = 0;
	T minY = getPoint(0).y;
	for (int i = 1; i < nPoints; i++) {
		if (getPoint(i).y < minY) minYIndex = i;
	}

	for (int offs = 0; offs < nPoints; offs++) {
		Polygon2dPoint<T> a = getPoint(minYIndex + offs);
		Polygon2dPoint<T> b = getPoint((minYIndex + offs + 1) % nPoints);
		Polygon2dPoint<T> c = getPoint((minYIndex + offs + 2) % nPoints);

		T det = (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
		if (det != 0) return det < 0;
	}
	return true;
}

template <typename T>
Polygon2dBounds<T> Polygon2d<T>::getBounds()
{
	Polygon2dBounds<T> bounds,
		Polygon2dPoint<T> p = getPoint(0);
	bounds.x0 = bounds.x1 = p.x;
	bounds.y0 = bounds.y1 = p.y;

	int nPoints = numPoints();
	for (int i = 1; i < nPoints; i++) {
		Polygon2dPoint<T> p = getPoint(i);
		if (p.x < bounds.x0) bounds.x0 = p.x;
		if (p.x > bounds.x1) bounds.x1 = p.x;
		if (p.y < bounds.y0) bounds.y0 = p.y;
		if (p.y > bounds.y1) bounds.y1 = p.y;
	}

	return bounds;
}

template <typename T>
bool Polygon2d<T>::contains(Polygon2dPoint<T> point, FillRule rule) {
	int count = 0, nPoints = numPoints();
	Polygon2dPoint<T> a = getPoint(nPoints - 1), b = getPoint(0);
	for (int i = 0; i < nPoints; i++, a = b) {
		b = getPoint(i);
		// skip horizontal segments
		if (a.y == b.y) continue;
		// skip if point is at same y as end of segment
		// (the next segment will be applied instead)
		if (point.y == b.y) continue;
		// skip if point is trivially to the right of segment
		if (point.x > a.x && point.x > b.x) continue;
		// skip if point is above or below the segment
		if ((point.y >= a.y) == (point.y >= b.y)) continue;
		// skip if point is non-trivially to the right of segment
		if ((point.x - a.x) * (b.y - a.y) > (b.x - a.x) * (point.y - a.y)) continue;
		// a ray extending right from the point crosses the segment;
		// for evenodd, we just toggle on every crossing
		if (rule == nonzero) count = ~count;
		// for nonzero, we increment if the segment crosses going up
		else if (a.y > by) count++;
		// and decrement if the segment crosses going down.
		else count--;
		// (up/down may be backward depending on the handedness of the
		// coordinate system, but the algorithm gives the same result)
	}

	return count != 0;
}

