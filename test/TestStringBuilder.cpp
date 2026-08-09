#include "../UnitTesting.h"
#include "../Core.h"
using namespace SimpleLib;

Fact("StringBuilder Finish Returns Null Terminated String")
{
	StringBuilder<char> sb;
	sb.Append("Hello");
	sb.Append(' ');
	sb.Append("World");

	char* psz = sb.Finish();
	Assert(strcmp(psz, "Hello World") == 0);
}

Fact("StringBuilder Finish With Length Out Param")
{
	StringBuilder<char> sb;
	sb.Append("Hello World");

	int length;
	char* psz = sb.Finish(&length);
	Assert(length == 11);
	Assert(strcmp(psz, "Hello World") == 0);
}

Fact("StringBuilder Finish Does Not Reset Builder")
{
	StringBuilder<char> sb;
	sb.Append("Hello");

	// Calling Finish() twice should return the same buffer/content rather
	// than resetting the builder
	char* p1 = sb.Finish();
	char* p2 = sb.Finish();
	Assert(p1 == p2);
	Assert(strcmp(p1, "Hello") == 0);
}

Fact("StringBuilder GetBuffer Resets And Reserves")
{
	StringBuilder<char> sb;
	sb.Append("Old content that should be discarded");

	char* buf = sb.GetBuffer(5);
	Assert(buf != nullptr);
	Assert(sb.GetLength() == 5);

	memcpy(buf, "Howdy", 5);
	Assert(strncmp(sb.Finish(), "Howdy", 5) == 0);
}

Fact("StringBuilder GetBuffer Then Append")
{
	StringBuilder<char> sb;
	sb.Append("Discard me");

	char* buf = sb.GetBuffer(3);
	memcpy(buf, "abc", 3);
	sb.Append("def");

	Assert(strcmp(sb.Finish(), "abcdef") == 0);
}

Fact("StringBuilder ToString Returns Content As StringCore")
{
	StringBuilder<char> sb;
	sb.Append("Hello World");

	String str = sb.ToString();
	Assert(str.IsEqualTo("Hello World"));
	Assert(str.GetLength() == 11);
}

Fact("StringBuilder SyncLen Adopts Nul Terminated Length Of Content")
{
	StringBuilder<char> sb;

	// Reserve room for more than we actually write, then write a shorter
	// nul terminated string directly into the buffer (as eg: snprintf would)
	char* buf = sb.GetBuffer(20);
	strcpy(buf, "abc");

	// Length is still 20 until we resync it against the nul terminator
	Assert(sb.GetLength() == 20);

	sb.SyncLen();
	Assert(sb.GetLength() == 3);
	Assert(sb.ToString().IsEqualTo("abc"));
}

Fact("StringBuilder SyncLen Returns Self For Chaining")
{
	StringBuilder<char> sb;
	char* buf = sb.GetBuffer(20);
	strcpy(buf, "chained");

	String str = sb.SyncLen().ToString();
	Assert(str.IsEqualTo("chained"));
}
