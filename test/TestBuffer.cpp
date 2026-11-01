#include "../UnitTesting.h"
#include "../Core.h"
#include "../Core/Buffer.h"
using namespace SimpleLib;

Fact("Buffer default is empty")
{
	Buffer<int> buf;
	Assert(buf.GetCapacity() == 0);
	Assert((int*)buf == nullptr);
}

Fact("Buffer EnsureSize grows and preserves contents")
{
	Buffer<int> buf(4);
	Assert(buf.GetCapacity() == 4);
	for (int i = 0; i < 4; i++)
		buf[i] = i * 10;

	buf.EnsureSize(100);
	Assert(buf.GetCapacity() == 100);
	for (int i = 0; i < 4; i++)
		Assert(buf[i] == i * 10);

	// Never shrinks
	buf.EnsureSize(10);
	Assert(buf.GetCapacity() == 100);
}

Fact("Buffer SetCapacity zero frees")
{
	Buffer<char> buf(16);
	buf.SetCapacity(0);
	Assert(buf.GetCapacity() == 0);
	Assert((char*)buf == nullptr);
}
