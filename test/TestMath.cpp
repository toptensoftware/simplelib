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
