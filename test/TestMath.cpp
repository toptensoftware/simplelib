#include "../UnitTesting.h"
#include "../Geo.h"
using namespace SimpleLib;

Fact("Math::Min")
{
	Assert(Math::Min(10, 20) == 10);
}

Fact("Math::Max")
{
	Assert(Math::Max(10, 20) == 20);
}

Fact("Math::Normalize already in order")
{
	int a = 10;
	int b = 20;
	Math::Normalize(a, b);
	Assert(a == 10);
	Assert(b == 20);
}

Fact("Math::Normalize swaps out of order values")
{
	int a = 20;
	int b = 10;
	Math::Normalize(a, b);
	Assert(a == 10);
	Assert(b == 20);
}

Fact("Math::Normalize equal values")
{
	int a = 5;
	int b = 5;
	Math::Normalize(a, b);
	Assert(a == 5);
	Assert(b == 5);
}

Fact("Math::Normalize doubles")
{
	double a = 3.5;
	double b = 1.5;
	Math::Normalize(a, b);
	Assert(a == 1.5);
	Assert(b == 3.5);
}

Fact("Math::Constrain value within range")
{
	Assert(Math::Constrain(5, 0, 10) == 5);
}

Fact("Math::Constrain value below min")
{
	Assert(Math::Constrain(-5, 0, 10) == 0);
}

Fact("Math::Constrain value above max")
{
	Assert(Math::Constrain(15, 0, 10) == 10);
}

Fact("Math::Constrain value equal to min")
{
	Assert(Math::Constrain(0, 0, 10) == 0);
}

Fact("Math::Constrain value equal to max")
{
	Assert(Math::Constrain(10, 0, 10) == 10);
}

Fact("Math::Constrain inverse range value within range")
{
	Assert(Math::Constrain(5, 10, 0) == 5);
}

Fact("Math::Constrain inverse range value below max")
{
	Assert(Math::Constrain(-5, 10, 0) == 0);
}

Fact("Math::Constrain inverse range value above min")
{
	Assert(Math::Constrain(15, 10, 0) == 10);
}

Fact("Math::Constrain min equals max")
{
	Assert(Math::Constrain(5, 3, 3) == 3);
	Assert(Math::Constrain(3, 3, 3) == 3);
	Assert(Math::Constrain(-5, 3, 3) == 3);
}

Fact("Math::Constrain doubles")
{
	Assert(Math::Constrain(1.5, 0.0, 10.0) == 1.5);
	Assert(Math::Constrain(-1.5, 0.0, 10.0) == 0.0);
	Assert(Math::Constrain(11.5, 0.0, 10.0) == 10.0);
}
