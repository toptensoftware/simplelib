#include <cstdio>
#include <cstdlib>
#include "../UnitTesting.h"
#include "../Core.h"
#include "../FileSystem/FileSystem.h"
using namespace SimpleLib;

// Builds a scratch path in the system temp directory, distinct per test
static String TempPath(const char* name)
{
	const char* tempDir = getenv("TEMP");
	if (!tempDir)
		tempDir = getenv("TMP");
	if (!tempDir)
		tempDir = ".";
	return String::Format("%s\\simplelib_testfs_%s.tmp", tempDir, name);
}

// Creates a file with the given content using plain C stdio, independent of
// anything in the Stream module
static void WriteFile(const char* path, const char* content)
{
	FILE* f = fopen(path, "wb");
	fwrite(content, 1, strlen(content), f);
	fclose(f);
}

static String ReadFile(const char* path)
{
	FILE* f = fopen(path, "rb");
	Assert(f != nullptr);
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	StringBuilder<char> sb;
	char* buf = sb.Reserve((int)len);
	fread(buf, 1, len, f);
	fclose(f);
	return String(buf, (int)len);
}

Fact("FileSystem Exists")
{
	String path = TempPath("exists");
	remove(path.sz());

	Assert(!FileSystem::Exists(path.sz()));

	WriteFile(path.sz(), "content");
	Assert(FileSystem::Exists(path.sz()));

	remove(path.sz());
	Assert(!FileSystem::Exists(path.sz()));
}

Fact("FileSystem GetTempDirectory")
{
	String dir = FileSystem::GetTempDirectory();
	Assert(!dir.IsEmpty());
	Assert(FileSystem::Exists(dir.sz()));
}

Fact("FileSystem Rename")
{
	String oldPath = TempPath("rename_old");
	String newPath = TempPath("rename_new");
	remove(oldPath.sz());
	remove(newPath.sz());

	WriteFile(oldPath.sz(), "rename me");
	Assert(FileSystem::Rename(oldPath.sz(), newPath.sz()) == 0);

	Assert(!FileSystem::Exists(oldPath.sz()));
	Assert(FileSystem::Exists(newPath.sz()));
	Assert(ReadFile(newPath.sz()).IsEqualTo("rename me"));

	remove(newPath.sz());
}

Fact("FileSystem Unlink")
{
	String path = TempPath("unlink");
	WriteFile(path.sz(), "delete me");
	Assert(FileSystem::Exists(path.sz()));

	Assert(FileSystem::Unlink(path.sz()) == 0);
	Assert(!FileSystem::Exists(path.sz()));
}

Fact("FileSystem Stat")
{
	String path = TempPath("stat");
	remove(path.sz());
	WriteFile(path.sz(), "0123456789");	// 10 bytes

	Stat stat;
	Assert(FileSystem::Stat(path.sz(), &stat) == 0);
	Assert(stat.size == 10);
	Assert(stat.mtime > 0);

	remove(path.sz());
}

Fact("FileSystem Stat Missing File Fails")
{
	String path = TempPath("stat_missing");
	remove(path.sz());

	Stat stat;
	Assert(FileSystem::Stat(path.sz(), &stat) != 0);
}

Fact("FileSystem MakeDirectory")
{
	String dir = TempPath("mkdir_dir");
	RemoveDirectoryW(Encode<wchar_t>(dir.sz()).sz());	// clean slate, ignore result

	Assert(!FileSystem::Exists(dir.sz()));
	Assert(FileSystem::MakeDirectory(dir.sz()) == 0);
	Assert(FileSystem::Exists(dir.sz()));

	RemoveDirectoryW(Encode<wchar_t>(dir.sz()).sz());
}

Fact("FileSystem MakeDirectory Already Exists Fails")
{
	String dir = TempPath("mkdir_twice");
	RemoveDirectoryW(Encode<wchar_t>(dir.sz()).sz());	// clean slate, ignore result

	Assert(FileSystem::MakeDirectory(dir.sz()) == 0);
	Assert(FileSystem::MakeDirectory(dir.sz()) != 0);

	RemoveDirectoryW(Encode<wchar_t>(dir.sz()).sz());
}
