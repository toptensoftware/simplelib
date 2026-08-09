#include "../UnitTesting.h"
#include "../Core.h"
#include "../Stream/Stream.h"
#include "../Stream/MemoryStream.h"
using namespace SimpleLib;

Fact("MemoryStream Create Write Read")
{
	MemoryStream ms;
	Assert(ms.Create() == 0);

	const char data[] = "Hello World";
	Assert(ms.Write(data, sizeof(data)) == 0);
	Assert(ms.Length() == sizeof(data));

	ms.Seek(0, SEEK_SET);
	char buf[sizeof(data)];
	uint32_t cb;
	Assert(ms.Read(buf, sizeof(buf), &cb) == 0);
	Assert(cb == sizeof(data));
	Assert(memcmp(buf, data, sizeof(data)) == 0);
}

Fact("MemoryStream Write Grows Buffer")
{
	MemoryStream ms;
	ms.Create();

	// Larger than the initial 4096 byte allocation
	const int kSize = 10000;
	char* src = (char*)malloc(kSize);
	for (int i = 0; i < kSize; i++)
		src[i] = (char)(i & 0xFF);

	Assert(ms.Write(src, kSize) == 0);
	Assert(ms.Length() == kSize);
	Assert(memcmp(ms.GetBuffer(), src, kSize) == 0);

	free(src);
}

Fact("MemoryStream Open Existing Buffer Not Owned Is Read Only")
{
	char data[] = "Read only data";
	MemoryStream ms;
	Assert(ms.Open(data, sizeof(data), false) == 0);

	char buf[sizeof(data)];
	uint32_t cb;
	Assert(ms.Read(buf, sizeof(buf), &cb) == 0);
	Assert(cb == sizeof(data));
	Assert(memcmp(buf, data, sizeof(data)) == 0);

	// Not owned - writes must fail, and must not touch the caller's buffer
	Assert(ms.Write("x", 1) != 0);
	Assert(data[0] == 'R');
}

Fact("MemoryStream Init Copies Data")
{
	char src[] = "Hello";
	MemoryStream ms;
	Assert(ms.Init(src, 5) == 0);

	Assert(ms.Length() == 5);
	Assert(ms.GetBuffer() != (void*)src);	// must be an independent copy
	Assert(memcmp(ms.GetBuffer(), src, 5) == 0);

	// Mutating the source afterwards must not affect the stream's copy
	src[0] = 'X';
	Assert(memcmp(ms.GetBuffer(), "Hello", 5) == 0);
}

Fact("MemoryStream Init With Null Allocates Uninitialized Buffer")
{
	MemoryStream ms;
	Assert(ms.Init(nullptr, 100) == 0);
	Assert(ms.Length() == 100);
	Assert(ms.GetBuffer() != nullptr);
}

Fact("MemoryStream Read Past End Returns Partial")
{
	MemoryStream ms;
	ms.Create();
	ms.Write("0123456789", 10);
	ms.Seek(0, SEEK_SET);

	char buf[20];
	uint32_t cb;
	Assert(ms.Read(buf, 20, &cb) == 0);
	Assert(cb == 10);
	Assert(memcmp(buf, "0123456789", 10) == 0);
	Assert(ms.IsEof());
}

Fact("MemoryStream Read Exact Remaining Length")
{
	MemoryStream ms;
	ms.Create();
	ms.Write("0123456789", 10);
	ms.Seek(0, SEEK_SET);

	char buf[10];
	uint32_t cb;
	Assert(ms.Read(buf, 10, &cb) == 0);
	Assert(cb == 10);
	Assert(ms.IsEof());
}

Fact("MemoryStream Seek Set Cur End")
{
	MemoryStream ms;
	ms.Create();
	ms.Write("0123456789", 10);

	ms.Seek(3, SEEK_SET);
	Assert(ms.Tell() == 3);

	ms.Seek(2, SEEK_CUR);
	Assert(ms.Tell() == 5);

	// SEEK_END follows the standard fseek convention: position = length +
	// offset, so a negative offset moves back from the end
	ms.Seek(0, SEEK_END);
	Assert(ms.Tell() == 10);

	ms.Seek(-2, SEEK_END);
	Assert(ms.Tell() == 8);

	char ch;
	ms.Read(&ch, 1);
	Assert(ch == '8');
}

Fact("MemoryStream SetLength Truncates")
{
	MemoryStream ms;
	ms.Create();
	ms.Write("0123456789", 10);

	ms.Seek(5, SEEK_SET);
	Assert(ms.SetLength() == 0);

	Assert(ms.Length() == 5);
	Assert(memcmp(ms.GetBuffer(), "01234", 5) == 0);
}

Fact("MemoryStream IsEof")
{
	MemoryStream ms;
	ms.Create();
	ms.Write("Hi", 2);
	ms.Seek(0, SEEK_SET);

	Assert(!ms.IsEof());
	char buf[2];
	ms.Read(buf, 2);
	Assert(ms.IsEof());
}

Fact("MemoryStream IsEqual")
{
	MemoryStream a, b, c;
	a.Create();
	b.Create();
	c.Create();

	a.Write("Hello", 5);
	b.Write("Hello", 5);
	c.Write("World", 5);

	Assert(a.IsEqual(b));
	Assert(!a.IsEqual(c));

	MemoryStream emptyA, emptyB;
	emptyA.Create();
	emptyB.Create();
	Assert(emptyA.IsEqual(emptyB));
}

Fact("MemoryStream CloseAndDetach")
{
	MemoryStream ms;
	ms.Create();
	ms.Write("Detach me", 9);

	int64_t len = 0;
	void* p = ms.CloseAndDetach(&len);
	Assert(len == 9);
	Assert(memcmp(p, "Detach me", 9) == 0);
	free(p);
}

Fact("MemoryStream MoveFrom")
{
	MemoryStream a;
	a.Create();
	a.Write("Move me", 7);

	MemoryStream b;
	b.MoveFrom(a);

	Assert(b.Length() == 7);
	Assert(memcmp(b.GetBuffer(), "Move me", 7) == 0);
}

Fact("MemoryStream Save And Load")
{
	MemoryStream src;
	src.Create();
	src.Write("Payload data", 12);

	MemoryStream container;
	container.Create();
	Assert(src.Save(container) == 0);

	container.Seek(0, SEEK_SET);

	MemoryStream restored;
	Assert(restored.Load(container) == 0);

	Assert(restored.Length() == src.Length());
	Assert(restored.IsEqual(src));
}

Fact("MemoryStream WriteInt32 ReadInt32")
{
	MemoryStream ms;
	ms.Create();
	ms.WriteInt32(-12345);
	ms.WriteInt32(67890);

	ms.Seek(0, SEEK_SET);
	Assert(ms.ReadInt32() == -12345);
	Assert(ms.ReadInt32() == 67890);
}

Fact("MemoryStream WriteUInt32 ReadUInt32")
{
	MemoryStream ms;
	ms.Create();
	ms.WriteUInt32(0xFFFFFFFF);
	ms.WriteUInt32(42);

	ms.Seek(0, SEEK_SET);
	Assert(ms.ReadUInt32() == 0xFFFFFFFF);
	Assert(ms.ReadUInt32() == 42);
}

Fact("MemoryStream WriteString ReadString")
{
	MemoryStream ms;
	ms.Create();
	ms.WriteString("Hello World");
	ms.WriteString("");
	ms.WriteString(nullptr);

	ms.Seek(0, SEEK_SET);
	Assert(ms.ReadString().IsEqualTo("Hello World"));
	Assert(ms.ReadString().IsEmpty());
	Assert(ms.ReadString().IsEmpty());
}

Fact("MemoryStream ReadString At Buffer Boundary Does Not Overflow")
{
	// Regression test: ReadString() used to write one byte past the exact
	// number of characters it reserved in the StringBuilder, which was only
	// harmless by luck unless the length landed exactly on a capacity
	// boundary (e.g. StringBuilder's 128 byte short buffer).
	String longStr;
	{
		StringBuilder<char> sb;
		for (int i = 0; i < 128; i++)
			sb.Append((char)('A' + (i % 26)));
		longStr = sb.Finish();
	}
	Assert(longStr.GetLength() == 128);

	MemoryStream ms;
	ms.Create();
	ms.WriteString(longStr.sz());

	ms.Seek(0, SEEK_SET);
	String readBack = ms.ReadString();
	Assert(readBack.GetLength() == 128);
	Assert(readBack.IsEqualTo(longStr.sz()));
}

Fact("MemoryStream Copy Whole Stream")
{
	MemoryStream src;
	src.Create();
	src.Write("The quick brown fox", 20);
	src.Seek(0, SEEK_SET);

	MemoryStream dest;
	dest.Create();
	Assert(Stream::Copy(dest, src) == 0);

	Assert(dest.Length() == 20);
	Assert(dest.IsEqual(src));
}

Fact("MemoryStream Copy With Length Smaller Than Chunk Size")
{
	// Regression test: Copy(dest, src, length) used to clamp the wrong
	// variable, so whenever the remaining length was less than the internal
	// 4096 byte buffer it would over-read/over-write up to a full 4096 byte
	// block instead of stopping at the requested length.
	MemoryStream src;
	src.Create();
	char bigData[5000];
	for (int i = 0; i < 5000; i++)
		bigData[i] = (char)(i & 0xFF);
	src.Write(bigData, 5000);
	src.Seek(0, SEEK_SET);

	MemoryStream dest;
	dest.Create();
	Assert(Stream::Copy(dest, src, 100) == 0);

	Assert(dest.Length() == 100);
	Assert(memcmp(dest.GetBuffer(), bigData, 100) == 0);
}

Fact("MemoryStream Copy With Length Spanning Multiple Chunks")
{
	MemoryStream src;
	src.Create();
	char bigData[10000];
	for (int i = 0; i < 10000; i++)
		bigData[i] = (char)(i & 0xFF);
	src.Write(bigData, 10000);
	src.Seek(0, SEEK_SET);

	MemoryStream dest;
	dest.Create();
	Assert(Stream::Copy(dest, src, 9000) == 0);

	Assert(dest.Length() == 9000);
	Assert(memcmp(dest.GetBuffer(), bigData, 9000) == 0);
}
