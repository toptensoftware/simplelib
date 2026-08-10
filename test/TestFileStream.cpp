#include <cstdio>
#include <cstdlib>
#include "../UnitTesting.h"
#include "../Core.h"
#include "../Stream/Stream.h"
#include "../Stream/FileStream.h"
using namespace SimpleLib;

// Builds a scratch file path in the system temp directory, distinct per test
static String TempFilePath(const char* name)
{
	const char* tempDir = getenv("TEMP");
	if (!tempDir)
		tempDir = getenv("TMP");
	if (!tempDir)
		tempDir = ".";
	return String::Format("%s\\simplelib_teststream_%s.tmp", tempDir, name);
}

Fact("FileStream Create Write Read")
{
	String path = TempFilePath("create_write_read");

	FileStream fs;
	Assert(fs.Open(path.sz(), "wb+") == 0);

	const char data[] = "Hello World";
	Assert(fs.Write(data, sizeof(data)) == 0);

	fs.Seek(0, SEEK_SET);
	char buf[sizeof(data)];
	uint32_t cb;
	Assert(fs.Read(buf, sizeof(buf), &cb) == 0);
	Assert(cb == sizeof(data));
	Assert(memcmp(buf, data, sizeof(data)) == 0);

	fs.Close();
	remove(path.sz());
}

Fact("FileStream Open Existing For Read")
{
	String path = TempFilePath("open_existing");

	{
		FileStream fs;
		fs.Open(path.sz(), "wb+");
		fs.Write("Persisted", 9);
		fs.Close();
	}

	FileStream fs2;
	Assert(fs2.Open(path.sz(), "rb") == 0);

	char buf[9];
	uint32_t cb;
	Assert(fs2.Read(buf, sizeof(buf), &cb) == 0);
	Assert(cb == 9);
	Assert(memcmp(buf, "Persisted", 9) == 0);

	fs2.Close();
	remove(path.sz());
}

Fact("FileStream Open Missing File Fails")
{
	String path = TempFilePath("does_not_exist");
	remove(path.sz());	// make sure it really doesn't exist

	FileStream fs;
	Assert(fs.Open(path.sz(), "rb") != 0);
}

Fact("FileStream Seek Tell Length")
{
	String path = TempFilePath("seek_tell_length");

	FileStream fs;
	fs.Open(path.sz(), "wb+");
	fs.Write("0123456789", 10);
	Assert(fs.Length() == 10);

	fs.Seek(3, SEEK_SET);
	Assert(fs.Tell() == 3);

	fs.Seek(2, SEEK_CUR);
	Assert(fs.Tell() == 5);

	// SEEK_END follows the standard fseek convention: position = length +
	// offset, so a negative offset moves back from the end
	fs.Seek(0, SEEK_END);
	Assert(fs.Tell() == 10);

	fs.Seek(-2, SEEK_END);
	Assert(fs.Tell() == 8);

	char ch;
	fs.Read(&ch, 1);
	Assert(ch == '8');

	// Length() must not disturb the current position
	fs.Seek(4, SEEK_SET);
	Assert(fs.Length() == 10);
	Assert(fs.Tell() == 4);

	fs.Close();
	remove(path.sz());
}

Fact("FileStream SetLength Truncates")
{
	String path = TempFilePath("setlength_truncate");

	FileStream fs;
	fs.Open(path.sz(), "wb+");
	fs.Write("0123456789", 10);

	fs.Seek(5, SEEK_SET);
	Assert(fs.SetLength() == 0);
	Assert(fs.Length() == 5);

	fs.Seek(0, SEEK_SET);
	char buf[5];
	fs.Read(buf, 5);
	Assert(memcmp(buf, "01234", 5) == 0);

	fs.Close();
	remove(path.sz());
}

Fact("FileStream IsEof")
{
	String path = TempFilePath("iseof");

	FileStream fs;
	fs.Open(path.sz(), "wb+");
	fs.Write("Hi", 2);
	fs.Seek(0, SEEK_SET);

	Assert(!fs.IsEof());
	char buf[2];
	fs.Read(buf, 2);
	Assert(fs.IsEof());

	fs.Close();
	remove(path.sz());
}

Fact("FileStream WriteInt32 ReadInt32")
{
	String path = TempFilePath("int32");

	FileStream fs;
	fs.Open(path.sz(), "wb+");
	fs.WriteInt32(-12345);
	fs.WriteInt32(67890);

	fs.Seek(0, SEEK_SET);
	Assert(fs.ReadInt32() == -12345);
	Assert(fs.ReadInt32() == 67890);

	fs.Close();
	remove(path.sz());
}

Fact("FileStream WriteString ReadString")
{
	String path = TempFilePath("string");

	FileStream fs;
	fs.Open(path.sz(), "wb+");
	fs.WriteString("Hello World");
	fs.WriteString("");
	fs.WriteString(nullptr);

	fs.Seek(0, SEEK_SET);
	Assert(fs.ReadString().IsEqualTo("Hello World"));
	Assert(fs.ReadString().IsEmpty());
	Assert(fs.ReadString().IsEmpty());

	fs.Close();
	remove(path.sz());
}

Fact("FileStream Copy Whole Stream")
{
	String srcPath = TempFilePath("copy_src");
	String destPath = TempFilePath("copy_dest");

	FileStream src;
	src.Open(srcPath.sz(), "wb+");
	src.Write("The quick brown fox", 20);
	src.Seek(0, SEEK_SET);

	FileStream dest;
	dest.Open(destPath.sz(), "wb+");
	Assert(Stream::Copy(dest, src) == 0);

	Assert(dest.Length() == 20);
	dest.Seek(0, SEEK_SET);
	char buf[20];
	dest.Read(buf, 20);
	Assert(memcmp(buf, "The quick brown fox", 20) == 0);

	src.Close();
	dest.Close();
	remove(srcPath.sz());
	remove(destPath.sz());
}

Fact("FileStream Copy With Length Smaller Than Chunk Size")
{
	// Regression test for the same Copy(dest,src,length) clamp bug covered
	// in TestMemoryStream, exercised here through real file I/O too.
	String srcPath = TempFilePath("copy_len_src");
	String destPath = TempFilePath("copy_len_dest");

	FileStream src;
	src.Open(srcPath.sz(), "wb+");
	char bigData[5000];
	for (int i = 0; i < 5000; i++)
		bigData[i] = (char)(i & 0xFF);
	src.Write(bigData, 5000);
	src.Seek(0, SEEK_SET);

	FileStream dest;
	dest.Open(destPath.sz(), "wb+");
	Assert(Stream::Copy(dest, src, 100) == 0);

	Assert(dest.Length() == 100);
	dest.Seek(0, SEEK_SET);
	char buf[100];
	dest.Read(buf, 100);
	Assert(memcmp(buf, bigData, 100) == 0);

	src.Close();
	dest.Close();
	remove(srcPath.sz());
	remove(destPath.sz());
}

Fact("FileStream Reopen Preserves Content Across Close")
{
	String path = TempFilePath("reopen");

	{
		FileStream fs;
		fs.Open(path.sz(), "wb+");
		fs.WriteInt32(42);
		fs.Close();
	}

	{
		FileStream fs;
		Assert(fs.Open(path.sz(), "rb+") == 0);
		Assert(fs.ReadInt32() == 42);
		fs.Close();
	}

	remove(path.sz());
}
