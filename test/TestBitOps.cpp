#include "../UnitTesting.h"
#include "../Core.h"
using namespace SimpleLib;

Fact("BitOps is_unsigned_bitmask")
{
	Assert(is_unsigned_bitmask<uint8_t>::value);
	Assert(is_unsigned_bitmask<uint16_t>::value);
	Assert(is_unsigned_bitmask<uint32_t>::value);
	Assert(is_unsigned_bitmask<uint64_t>::value);
	Assert(!is_unsigned_bitmask<int8_t>::value);
	Assert(!is_unsigned_bitmask<int32_t>::value);
	Assert(!is_unsigned_bitmask<int64_t>::value);
}

Fact("BitOps bitCount")
{
	Assert(bitCount((uint32_t)0) == 0);
	Assert(bitCount((uint32_t)1) == 1);
	Assert(bitCount((uint8_t)0xFF) == 8);
	Assert(bitCount((uint32_t)0xB5) == 5);	// 1011 0101
	Assert(bitCount((uint32_t)0xFFFFFFFF) == 32);
	Assert(bitCount((uint64_t)0xFFFFFFFFFFFFFFFFULL) == 64);
	Assert(bitCount((uint16_t)0x8001) == 2);
}

Fact("BitOps bitSet")
{
	uint32_t n = 0;
	bitSet(n, 0);
	Assert(n == 1);

	bitSet(n, 3);
	Assert(n == 0x9);	// bits 0 and 3

	// Setting an already-set bit is a no-op
	bitSet(n, 3);
	Assert(n == 0x9);

	// High bit of a 64 bit value
	uint64_t n64 = 0;
	bitSet(n64, 63);
	Assert(n64 == 0x8000000000000000ULL);
}

Fact("BitOps bitClear")
{
	uint32_t n = 0xFF;
	bitClear(n, 0);
	Assert(n == 0xFE);

	// Clearing an already-clear bit is a no-op
	bitClear(n, 0);
	Assert(n == 0xFE);

	bitClear(n, 7);
	Assert(n == 0x7E);
}

Fact("BitOps bitSet With Bool")
{
	uint32_t n = 0;
	bitSet(n, 2, true);
	Assert(n == 0x4);

	bitSet(n, 2, false);
	Assert(n == 0);

	// Redundant calls are no-ops
	bitSet(n, 2, false);
	Assert(n == 0);
}

Fact("BitOps FindNthSetBit")
{
	uint32_t mask = 0xB5;	// bits set at 0, 2, 4, 5, 7
	Assert(FindNthSetBit(mask, 0) == 0);
	Assert(FindNthSetBit(mask, 1) == 2);
	Assert(FindNthSetBit(mask, 2) == 4);
	Assert(FindNthSetBit(mask, 3) == 5);
	Assert(FindNthSetBit(mask, 4) == 7);
	Assert(FindNthSetBit(mask, 5) == -1);	// No 6th set bit

	Assert(FindNthSetBit((uint32_t)0, 0) == -1);

	uint64_t mask64 = 0;
	bitSet(mask64, 40);
	Assert(FindNthSetBit(mask64, 0) == 40);
	Assert(FindNthSetBit(mask64, 1) == -1);
}
