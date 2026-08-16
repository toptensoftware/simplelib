#include "../UnitTesting.h"
#include "../Core.h"
using namespace SimpleLib;

Fact("Endian fourcc ToBig produces bytes in string order")
{
	uint32_t v = Endian::ToBig(Endian::fourcc("ABCD"));
	uint8_t* bytes = (uint8_t*)&v;
	Assert(bytes[0] == 'A');
	Assert(bytes[1] == 'B');
	Assert(bytes[2] == 'C');
	Assert(bytes[3] == 'D');
}

Fact("Endian fourcc FromBig of on-disk bytes matches fourcc")
{
	// Simulate reading the on-disk bytes (string order) back into memory
	uint8_t onDisk[4] = { 'A', 'B', 'C', 'D' };
	uint32_t raw;
	memcpy(&raw, onDisk, 4);

	Assert(Endian::FromBig(raw) == Endian::fourcc("ABCD"));
}
