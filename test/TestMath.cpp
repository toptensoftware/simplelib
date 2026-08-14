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
