#include "../UnitTesting.h"
#include "../Core.h"
using namespace SimpleLib;

Fact("Endian ByteSwap round trip")
{
	Assert(Endian::ByteSwap((uint16_t)0x0102) == 0x0201);
	Assert(Endian::ByteSwap((uint32_t)0x01020304) == 0x04030201);
	Assert(Endian::ByteSwap((uint64_t)0x0102030405060708ULL) == 0x0807060504030201ULL);
	Assert(Endian::ByteSwap((int16_t)0x0102) == (int16_t)0x0201);
	Assert(Endian::ByteSwap((int32_t)0x01020304) == (int32_t)0x04030201);
	Assert(Endian::ByteSwap((int64_t)0x0102030405060708LL) == (int64_t)0x0807060504030201LL);

	float f = 1.0f;
	float f2 = Endian::ByteSwap(Endian::ByteSwap(f));
	Assert(f == f2);

	double d = 1.0;
	double d2 = Endian::ByteSwap(Endian::ByteSwap(d));
	Assert(d == d2);
}

Fact("Endian ToBig/FromBig/ToLittle/FromLittle round trip for all types")
{
	Assert(Endian::FromBig(Endian::ToBig((int16_t)0x1234)) == (int16_t)0x1234);
	Assert(Endian::FromBig(Endian::ToBig((int32_t)0x12345678)) == (int32_t)0x12345678);
	Assert(Endian::FromBig(Endian::ToBig((int64_t)0x123456789ABCDEF0LL)) == (int64_t)0x123456789ABCDEF0LL);
	Assert(Endian::FromLittle(Endian::ToLittle((int16_t)0x1234)) == (int16_t)0x1234);
	Assert(Endian::FromLittle(Endian::ToLittle((int32_t)0x12345678)) == (int32_t)0x12345678);
	Assert(Endian::FromLittle(Endian::ToLittle((int64_t)0x123456789ABCDEF0LL)) == (int64_t)0x123456789ABCDEF0LL);

	Assert(Endian::FromBig(Endian::ToBig(1.5f)) == 1.5f);
	Assert(Endian::FromBig(Endian::ToBig(1.5)) == 1.5);
}
