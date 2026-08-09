#include "../UnitTesting.h"
#include "../Core.h"
using namespace SimpleLib;

// Expected values are the standard CRC-32/ISO-HDLC checksums (poly 0x04C11DB7,
// reflected in/out, init/xorout 0xFFFFFFFF) - the same algorithm used by zip,
// gzip, PNG and Ethernet - independently verified against a reference
// implementation rather than taken on faith.

Fact("Crc32 Empty Input")
{
	Assert(Crc32::Calculate(nullptr, 0) == 0x00000000);

	Crc32 crc;
	Assert(crc.Finish() == 0x00000000);
}

Fact("Crc32 Standard Check Value")
{
	// The official CRC-32 catalogue "check" value for this algorithm
	Assert(Crc32::Calculate("123456789", 9) == 0xCBF43926);
}

Fact("Crc32 Known Strings")
{
	Assert(Crc32::Calculate("Hello", 5) == 0xF7D18982);
	Assert(Crc32::Calculate("World", 5) == 0xFBB63E47);
	Assert(Crc32::Calculate("abc", 3) == 0x352441C2);
	Assert(Crc32::Calculate("xyz", 3) == 0xEB8EBA67);
}

Fact("Crc32 Pangram")
{
	const char* text = "The quick brown fox jumps over the lazy dog";
	Assert(Crc32::Calculate(text, (int)strlen(text)) == 0x414FA339);
}

Fact("Crc32 Different Data Produces Different Checksum")
{
	Assert(Crc32::Calculate("Hello", 5) != Crc32::Calculate("World", 5));
}

Fact("Crc32 Per Byte Update Matches Bulk Update")
{
	const char* text = "Hello World";
	int len = (int)strlen(text);

	Crc32 crcBulk;
	crcBulk.Update(text, len);

	Crc32 crcPerByte;
	for (int i = 0; i < len; i++)
		crcPerByte.Update((unsigned char)text[i]);

	Assert(crcBulk.Finish() == crcPerByte.Finish());
	Assert(crcBulk.Finish() == 0x4A17B156);
	Assert(crcBulk.Finish() == Crc32::Calculate(text, len));
}

Fact("Crc32 Split Across Multiple Update Calls")
{
	Crc32 crc;
	crc.Update("Hello", 5);
	crc.Update(" World", 6);
	Assert(crc.Finish() == 0x4A17B156);
	Assert(crc.Finish() == Crc32::Calculate("Hello World", 11));
}

Fact("Crc32 Reset Reuses Instance")
{
	Crc32 crc;
	crc.Update("abc", 3);
	Assert(crc.Finish() == 0x352441C2);

	crc.Reset();
	crc.Update("xyz", 3);
	Assert(crc.Finish() == 0xEB8EBA67);
}

Fact("Crc32 Multiple Instances Are Independent")
{
	Crc32 crc1;
	Crc32 crc2;

	// Interleave updates to make sure the two instances don't share state
	crc1.Update("a", 1);
	crc2.Update("x", 1);
	crc1.Update("bc", 2);
	crc2.Update("yz", 2);

	Assert(crc1.Finish() == 0x352441C2);
	Assert(crc2.Finish() == 0xEB8EBA67);
}
