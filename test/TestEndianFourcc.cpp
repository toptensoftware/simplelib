#include "../UnitTesting.h"
#include "../Core.h"
using namespace SimpleLib;

Fact("Endian fourcc produces bytes in string order in memory")
{
	uint32_t v = Endian::fourcc("ABCD");
	uint8_t* bytes = (uint8_t*)&v;
	Assert(bytes[0] == 'A');
	Assert(bytes[1] == 'B');
	Assert(bytes[2] == 'C');
	Assert(bytes[3] == 'D');
}

Fact("Endian fourcc matches a raw memcpy of the string bytes")
{
	char onDisk[4] = { 'A', 'B', 'C', 'D' };
	uint32_t raw;
	memcpy(&raw, onDisk, 4);

	Assert(raw == Endian::fourcc("ABCD"));
}
