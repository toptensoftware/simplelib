#include "../UnitTesting.h"
#include "../Core.h"
#include "../Path/Path.h"
using namespace SimpleLib;

// Exercise both semantics regardless of the host platform
typedef PathCore<SPathSemanticsWindows> WinPath;
typedef PathCore<SPathSemanticsPosix> PosixPath;

Fact("Path::Join basic")
{
    Assert(PosixPath::Join("a", "b").IsEqualTo("a/b"));
    Assert(PosixPath::Join("a/", "b").IsEqualTo("a/b"));
    Assert(PosixPath::Join("a", "/b").IsEqualTo("a/b"));
    Assert(PosixPath::Join("a/", "/b").IsEqualTo("a/b"));
    Assert(PosixPath::Join("", "b").IsEqualTo("b"));
    Assert(PosixPath::Join("a", "").IsEqualTo("a"));
}

Fact("Path::GetDirectoryName")
{
    Assert(PosixPath::GetDirectoryName("/a/b/c.txt").IsEqualTo("/a/b"));
    Assert(PosixPath::GetDirectoryName("a/b").IsEqualTo("a"));
    Assert(PosixPath::GetDirectoryName("/foo").IsEqualTo("/"));
    Assert(PosixPath::GetDirectoryName("/").IsNull());
    Assert(PosixPath::GetDirectoryName("foo").IsNull());
    Assert(PosixPath::GetDirectoryName("a/b/").IsEqualTo("a/b"));
    Assert(WinPath::GetDirectoryName("C:\\foo").IsEqualTo("C:\\"));
    Assert(WinPath::GetDirectoryName("C:\\foo\\bar").IsEqualTo("C:\\foo"));
    Assert(WinPath::GetDirectoryName("C:\\").IsNull());
}

Fact("Path::GetFileName")
{
    Assert(PosixPath::GetFileName("/a/b/c.txt").IsEqualTo("c.txt"));
    Assert(PosixPath::GetFileName("c.txt").IsEqualTo("c.txt"));
    Assert(PosixPath::GetFileName("/a/b/").IsEqualTo(""));
}

Fact("Path::GetFileNameWithoutExtension")
{
    Assert(PosixPath::GetFileNameWithoutExtension("/a/b/c.txt").IsEqualTo("c"));
    Assert(PosixPath::GetFileNameWithoutExtension("c.txt").IsEqualTo("c"));
    Assert(PosixPath::GetFileNameWithoutExtension("c").IsEqualTo("c"));
    Assert(PosixPath::GetFileNameWithoutExtension("/a.b/c").IsEqualTo("c"));
    Assert(PosixPath::GetFileNameWithoutExtension("archive.tar.gz").IsEqualTo("archive.tar"));
}

Fact("Path::GetExtension / ChangeExtension")
{
    Assert(PosixPath::GetExtension("a/b.txt").IsEqualTo(".txt"));
    Assert(PosixPath::GetExtension("a/b").IsNull());
    Assert(PosixPath::GetExtension("a.b/c").IsNull());
    Assert(PosixPath::ChangeExtension("a/b.txt", "md").IsEqualTo("a/b.md"));
    Assert(PosixPath::ChangeExtension("a/b", "md").IsEqualTo("a/b.md"));
    Assert(PosixPath::ChangeExtension("a/b.txt", ".md").IsEqualTo("a/b.md"));
}

Fact("Path::Canonicalize posix")
{
    Assert(PosixPath::Canonicalize("a/./b").IsEqualTo("a/b"));
    Assert(PosixPath::Canonicalize("a//b").IsEqualTo("a/b"));
    Assert(PosixPath::Canonicalize("a/b/../c").IsEqualTo("a/c"));
    Assert(PosixPath::Canonicalize("/a/b/../../c").IsEqualTo("/c"));
    Assert(PosixPath::Canonicalize("/a/../../c").IsEqualTo("/c"));
    Assert(PosixPath::Canonicalize("a/b/../../../c").IsEqualTo("../c"));
    Assert(PosixPath::Canonicalize("../../a").IsEqualTo("../../a"));
    Assert(PosixPath::Canonicalize("./a").IsEqualTo("a"));
    Assert(PosixPath::Canonicalize(".").IsEqualTo("."));
    Assert(PosixPath::Canonicalize("").IsEqualTo(""));
    Assert(PosixPath::Canonicalize("a/b/").IsEqualTo("a/b/"));
    Assert(PosixPath::Canonicalize("/").IsEqualTo("/"));
}

Fact("Path::Canonicalize windows")
{
    Assert(WinPath::Canonicalize("C:\\a\\.\\b").IsEqualTo("C:\\a\\b"));
    Assert(WinPath::Canonicalize("C:\\a\\b\\..").IsEqualTo("C:\\a"));
    Assert(WinPath::Canonicalize("C:\\a\\..").IsEqualTo("C:\\"));
    Assert(WinPath::Canonicalize("C:\\a\\b\\..\\..\\..").IsEqualTo("C:\\"));
    Assert(WinPath::Canonicalize("a/b\\c").IsEqualTo("a\\b\\c"));
    Assert(WinPath::Canonicalize("\\\\server\\share\\a\\..\\b").IsEqualTo("\\\\server\\share\\b"));
}

Fact("Path::Combine")
{
    Assert(PosixPath::Combine("/a/b", "c/d").IsEqualTo("/a/b/c/d"));
    Assert(PosixPath::Combine("/a/b", "../c").IsEqualTo("/a/c"));
    Assert(PosixPath::Combine("/a/b", "/c").IsEqualTo("/c"));
    Assert(WinPath::Combine("C:\\a\\b", "c").IsEqualTo("C:\\a\\b\\c"));
    Assert(WinPath::Combine("C:\\a\\b", "\\c").IsEqualTo("C:\\c"));
}

Fact("Path::GetPrefixLength (semantics driven, host independent)")
{
    Assert(WinPath::GetPrefixLength("C:\\foo") == 2);
    Assert(WinPath::GetPrefixLength("c:foo") == 2);
    Assert(WinPath::GetPrefixLength("\\\\server\\share\\x") == 14);
    Assert(WinPath::GetPrefixLength("\\\\server\\share") == 14);
    Assert(WinPath::GetPrefixLength("/foo/bar") == 0);
    Assert(WinPath::GetPrefixLength("1:foo") == 0);   // not a drive letter
    Assert(PosixPath::GetPrefixLength("C:\\foo") == 0);
    Assert(PosixPath::GetPrefixLength("/foo") == 0);
}

Fact("Path::IsFullyQualified (semantics driven, host independent)")
{
    Assert(WinPath::IsFullyQualified("C:\\foo"));
    Assert(!WinPath::IsFullyQualified("C:foo"));
    Assert(!WinPath::IsFullyQualified("foo\\bar"));
    Assert(WinPath::IsFullyQualified("\\\\server\\share\\x"));
    Assert(!WinPath::IsFullyQualified(""));
    Assert(PosixPath::IsFullyQualified("/foo"));
    Assert(!PosixPath::IsFullyQualified("foo/bar"));
    Assert(!PosixPath::IsFullyQualified("C:\\foo"));   // no drive concept on posix
}

Fact("Path::GetCurrentDirectory / GetFullPath smoke")
{
    String cwd = Path::GetCurrentDirectory();
    Assert(!cwd.IsEmpty());
    Assert(Path::IsFullyQualified(cwd));

    String full = Path::GetFullPath("sub/file.txt");
    Assert(Path::IsFullyQualified(full));
    Assert(full.EndsWith("file.txt"));

    String abs = Path::GetFullPath(Path::Join(cwd, "a/./b/../c"));
    Assert(abs.EndsWith(Path::Join("a", "c")));
}

Fact("Path::DoesMatchPattern")
{
    Assert(PosixPath::DoesMatchPattern<char>("foo.txt", "*.txt"));
    Assert(PosixPath::DoesMatchPattern<char>("foo.txt", "*.txt;*.md"));
    Assert(PosixPath::DoesMatchPattern<char>("foo.md", "*.txt;*.md"));
    Assert(!PosixPath::DoesMatchPattern<char>("foo.dat", "*.txt;*.md"));
    Assert(PosixPath::DoesMatchPattern<char>("foo.txt", "foo.???"));
    Assert(PosixPath::DoesMatchPattern<char>("anything", "*;*.md"));
    Assert(WinPath::DoesMatchPattern<char>("Foo.TXT", "*.txt"));      // case insensitive
    Assert(!PosixPath::DoesMatchPattern<char>("Foo.TXT", "*.txt"));   // case sensitive
}
