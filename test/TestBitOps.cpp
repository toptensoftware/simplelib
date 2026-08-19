#include "../UnitTesting.h"
#include "../Core.h"
using namespace SimpleLib;

Fact("Bit is_unsigned_bitmask")
{
	Assert(is_unsigned_bitmask<uint8_t>::value);
	Assert(is_unsigned_bitmask<uint16_t>::value);
	Assert(is_unsigned_bitmask<uint32_t>::value);
	Assert(is_unsigned_bitmask<uint64_t>::value);
	Assert(!is_unsigned_bitmask<int8_t>::value);
	Assert(!is_unsigned_bitmask<int32_t>::value);
	Assert(!is_unsigned_bitmask<int64_t>::value);
}

Fact("Bit Count")
{
	Assert(Bit::Count((uint32_t)0) == 0);
	Assert(Bit::Count((uint32_t)1) == 1);
	Assert(Bit::Count((uint8_t)0xFF) == 8);
	Assert(Bit::Count((uint32_t)0xB5) == 5);	// 1011 0101
	Assert(Bit::Count((uint32_t)0xFFFFFFFF) == 32);
	Assert(Bit::Count((uint64_t)0xFFFFFFFFFFFFFFFFULL) == 64);
	Assert(Bit::Count((uint16_t)0x8001) == 2);
}

Fact("Bit Set")
{
	uint32_t n = 0;
	Bit::Set(n, 0);
	Assert(n == 1);

	Bit::Set(n, 3);
	Assert(n == 0x9);	// bits 0 and 3

	// Setting an already-set bit is a no-op
	Bit::Set(n, 3);
	Assert(n == 0x9);

	// High bit of a 64 bit value
	uint64_t n64 = 0;
	Bit::Set(n64, 63);
	Assert(n64 == 0x8000000000000000ULL);
}

Fact("BitOps Bit::Clear")
{
	uint32_t n = 0xFF;
	Bit::Clear(n, 0);
	Assert(n == 0xFE);

	// Clearing an already-clear bit is a no-op
	Bit::Clear(n, 0);
	Assert(n == 0xFE);

	Bit::Clear(n, 7);
	Assert(n == 0x7E);
}

Fact("BitOps Bit::Set With Bool")
{
	uint32_t n = 0;
	Bit::Set(n, 2, true);
	Assert(n == 0x4);

	Bit::Set(n, 2, false);
	Assert(n == 0);

	// Redundant calls are no-ops
	Bit::Set(n, 2, false);
	Assert(n == 0);
}

Fact("BitOps Bit::Find")
{
	uint32_t mask = 0xB5;	// bits set at 0, 2, 4, 5, 7
	Assert(Bit::Find(mask, 0) == 0);
	Assert(Bit::Find(mask, 1) == 2);
	Assert(Bit::Find(mask, 2) == 4);
	Assert(Bit::Find(mask, 3) == 5);
	Assert(Bit::Find(mask, 4) == 7);
	Assert(Bit::Find(mask, 5) == -1);	// No 6th set bit

	Assert(Bit::Find((uint32_t)0, 0) == -1);

	uint64_t mask64 = 0;
	Bit::Set(mask64, 40);
	Assert(Bit::Find(mask64, 0) == 40);
	Assert(Bit::Find(mask64, 1) == -1);
}
