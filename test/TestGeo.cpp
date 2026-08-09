#include "../UnitTesting.h"
#include "../Geo.h"
using namespace SimpleLib;

Fact("Vector Equality")
{
	VectorI a(10, 10);
	VectorI b(10, 10);
	VectorI c(1,2);
	Assert(a == b);
	Assert(a != c);
}

Fact("Vector Magnitude")
{
	Assert(VectorD(0, 10).Magnitude() == 10);
}

Fact("Vector Distance")
{
	Assert(VectorD::Distance(VectorD(10, 10), VectorD(20, 10)) == 10);
}

Fact("Rectangle Area")
{
	Assert(RectangleD(10, 10, 20, 20).Area() == 400.0);
	Assert(RectangleF(10, 10, 20, 20).Area() == 400.0F);
	Assert(RectangleI(10, 10, 20, 20).Area() == 400);
}

Fact("Rectangle Equality")
{
	RectangleI a(10, 10, 20, 20);
	RectangleI b(10, 10, 20, 20);
	RectangleI c(1,2,3,4);
	Assert(a == b);
	Assert(a != c);
}