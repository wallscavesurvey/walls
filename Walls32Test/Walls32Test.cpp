#include "pch.h"
#include "CppUnitTest.h"
#include "Polygon2d.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Walls32Test
{
	typedef struct tagPOINT
	{
		long  x;
		long  y;
	} POINT;

	class TestPolygon :
		public Polygon2d<long>
	{
	public:
		TestPolygon(int nPoints, POINT* pPoints) : nPoints(nPoints), pPoints(pPoints) {}
		inline int numPoints() { return nPoints; }
		inline Polygon2dPoint<long> getPoint(int index) {
			POINT* pPoint = pPoints + index;
			return Polygon2dPoint<long> { pPoint->x, pPoint->y };
		}
	private:
		int nPoints;
		POINT* pPoints;
	};


	TEST_CLASS(Walls32Test)
	{
	public:
		TEST_METHOD(TestClockwiseTriangle)
		{
			POINT points[] = {
				{0, 0},
				{1, 0},
				{1, 1},
			};
			TestPolygon poly(5, points);
			Assert::AreEqual(true, poly.isClockwise());
		}
		TEST_METHOD(TestCounterClockwiseTriangle)
		{
			POINT points[] = {
				{0, 0},
				{1, 1},
				{1, 0},
			};
			TestPolygon poly(5, points);
			Assert::AreEqual(true, poly.isClockwise());
		}
	
	};
}
