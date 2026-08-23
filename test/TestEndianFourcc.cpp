#include "../UnitTesting.h"
#include "../Core.h"
using namespace SimpleLib;

Fact("Endian fourcc produces bytes in string order in memory")
{
	uint32_t v = cc4("ABCD");
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

	Assert(raw == cc4("ABCD"));
}

Fact("Endian cc8 produces bytes in string order in memory")
{
	uint64_t v = cc8("ABCDEFGH");
	uint8_t* bytes = (uint8_t*)&v;
	Assert(bytes[0] == 'A');
	Assert(bytes[1] == 'B');
	Assert(bytes[2] == 'C');
	Assert(bytes[3] == 'D');
	Assert(bytes[4] == 'E');
	Assert(bytes[5] == 'F');
	Assert(bytes[6] == 'G');
	Assert(bytes[7] == 'H');
}

Fact("Endian cc8 matches a raw memcpy of the string bytes")
{
	char onDisk[8] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H' };
	uint64_t raw;
	memcpy(&raw, onDisk, 8);

	Assert(raw == cc8("ABCDEFGH"));
}
