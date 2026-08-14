#include "../UnitTesting.h"
#include "../Core.h"
using namespace SimpleLib;

Fact("Parse IsWhiteSpace")
{
	Assert(Parse<char>::IsWhiteSpace(' '));
	Assert(Parse<char>::IsWhiteSpace('\t'));
	Assert(Parse<char>::IsWhiteSpace('\r'));
	Assert(Parse<char>::IsWhiteSpace('\n'));
	Assert(!Parse<char>::IsWhiteSpace('x'));
	Assert(!Parse<char>::IsWhiteSpace('\0'));
}

Fact("Parse SkipWhiteSpace")
{
	const char* p = "   \t\r\nabc";
	Assert(Parse<char>::SkipWhiteSpace(p));
	Assert(strcmp(p, "abc") == 0);

	// Nothing to skip
	const char* p2 = "abc";
	Assert(!Parse<char>::SkipWhiteSpace(p2));
	Assert(p2[0] == 'a');
}

Fact("Parse IsEOL")
{
	Assert(Parse<char>::IsEOL('\r'));
	Assert(Parse<char>::IsEOL('\n'));
	Assert(!Parse<char>::IsEOL('x'));
	Assert(!Parse<char>::IsEOL(' '));
}

Fact("Parse SkipEOL CRLF")
{
	const char* p = "\r\nrest";
	Assert(Parse<char>::SkipEOL(p));
	Assert(strcmp(p, "rest") == 0);
}

Fact("Parse SkipEOL LF only")
{
	const char* p = "\nrest";
	Assert(Parse<char>::SkipEOL(p));
	Assert(strcmp(p, "rest") == 0);
}

Fact("Parse SkipEOL CR only")
{
	const char* p = "\rrest";
	Assert(Parse<char>::SkipEOL(p));
	Assert(strcmp(p, "rest") == 0);
}

Fact("Parse SkipEOL no eol")
{
	const char* p = "rest";
	Assert(!Parse<char>::SkipEOL(p));
	Assert(p[0] == 'r');
}

Fact("Parse SkipToEOL")
{
	const char* p = "abc\r\ndef";
	Parse<char>::SkipToEOL(p);
	Assert(p[0] == '\r');

	// Runs to the null terminator when there's no EOL
	const char* p2 = "abcdef";
	Parse<char>::SkipToEOL(p2);
	Assert(p2[0] == '\0');
}

Fact("Parse SkipToNextLine")
{
	const char* p = "line1\r\nline2\nline3";
	Parse<char>::SkipToNextLine(p);
	Assert(strncmp(p, "line2", 5) == 0);

	Parse<char>::SkipToNextLine(p);
	Assert(strncmp(p, "line3", 5) == 0);
}

Fact("Parse IsOneOf")
{
	Assert(Parse<char>::IsOneOf('b', "abc"));
	Assert(!Parse<char>::IsOneOf('z', "abc"));
	Assert(!Parse<char>::IsOneOf('a', nullptr));
}

Fact("Parse IsIdentifierLeadChar")
{
	Assert(Parse<char>::IsIdentifierLeadChar('a', nullptr));
	Assert(Parse<char>::IsIdentifierLeadChar('Z', nullptr));
	Assert(Parse<char>::IsIdentifierLeadChar('_', nullptr));
	Assert(!Parse<char>::IsIdentifierLeadChar('0', nullptr));
	Assert(!Parse<char>::IsIdentifierLeadChar('$', nullptr));

	// Extra allowed lead characters
	Assert(Parse<char>::IsIdentifierLeadChar('$', "$@"));
	Assert(Parse<char>::IsIdentifierLeadChar('@', "$@"));
	Assert(!Parse<char>::IsIdentifierLeadChar('%', "$@"));
}

Fact("Parse IsIdentifierChar")
{
	Assert(Parse<char>::IsIdentifierChar('a', nullptr));
	Assert(Parse<char>::IsIdentifierChar('0', nullptr));
	Assert(Parse<char>::IsIdentifierChar('_', nullptr));
	Assert(!Parse<char>::IsIdentifierChar('-', nullptr));

	// Extra allowed non-lead characters
	Assert(Parse<char>::IsIdentifierChar('-', "-"));
}

Fact("Parse IsDigit")
{
	Assert(Parse<char>::IsDigit('0'));
	Assert(Parse<char>::IsDigit('9'));
	Assert(!Parse<char>::IsDigit('a'));
}

Fact("Parse ParseIdentifier")
{
	const char* p = "_foo123 bar";
	String str;
	Assert(Parse<char>::ParseIdentifier(p, str));
	Assert(str.IsEqualTo("_foo123"));
	Assert(p[0] == ' ');

	// Doesn't start with an identifier lead char
	const char* p2 = "123abc";
	String str2;
	Assert(!Parse<char>::ParseIdentifier(p2, str2));
	Assert(p2[0] == '1');
}

Fact("Parse ParseIdentifier with other chars")
{
	const char* p = "foo-bar!rest";
	String str;
	Assert(Parse<char>::ParseIdentifier(p, str, nullptr, "-"));
	Assert(str.IsEqualTo("foo-bar"));
	Assert(p[0] == '!');
}

Fact("Parse ParseInteger positive")
{
	const char* p = "12345rest";
	int val = 0;
	Assert(Parse<char>::ParseInteger(p, val));
	Assert(val == 12345);
	Assert(strcmp(p, "rest") == 0);
}

Fact("Parse ParseInteger negative")
{
	const char* p = "-42rest";
	int val = 0;
	Assert(Parse<char>::ParseInteger(p, val));
	Assert(val == -42);
	Assert(strcmp(p, "rest") == 0);
}

Fact("Parse ParseInteger explicit positive sign")
{
	const char* p = "+42rest";
	int val = 0;
	Assert(Parse<char>::ParseInteger(p, val));
	Assert(val == 42);
	Assert(strcmp(p, "rest") == 0);
}

Fact("Parse ParseInteger no digits")
{
	const char* p = "abc";
	int val = 0;
	Assert(!Parse<char>::ParseInteger(p, val));
	Assert(p[0] == 'a');		// Position unchanged on failure

	// Sign with no following digit also fails and rewinds
	const char* p2 = "-abc";
	int val2 = 0;
	Assert(!Parse<char>::ParseInteger(p2, val2));
	Assert(p2[0] == '-');
}

Fact("Parse ParseHexDigit")
{
	uint8_t val;
	Assert(Parse<char>::ParseHexDigit('0', val) && val == 0);
	Assert(Parse<char>::ParseHexDigit('9', val) && val == 9);
	Assert(Parse<char>::ParseHexDigit('a', val) && val == 0xA);
	Assert(Parse<char>::ParseHexDigit('f', val) && val == 0xF);
	Assert(Parse<char>::ParseHexDigit('A', val) && val == 0xA);
	Assert(Parse<char>::ParseHexDigit('F', val) && val == 0xF);
	Assert(!Parse<char>::ParseHexDigit('g', val));
	Assert(!Parse<char>::ParseHexDigit('G', val));
}

Fact("Parse ParseHexInteger")
{
	const char* p = "1A2Brest";
	uint32_t val = 0;
	Assert(Parse<char>::ParseHexInteger(p, val));
	Assert(val == 0x1A2Bu);
	Assert(strcmp(p, "rest") == 0);
}

Fact("Parse ParseHexInteger no digits")
{
	const char* p = "zzz";
	uint32_t val = 0;
	Assert(!Parse<char>::ParseHexInteger(p, val));
	Assert(p[0] == 'z');		// Position unchanged on failure
}

Fact("Parse ParseHexInteger null pointer")
{
	const char* p = nullptr;
	uint32_t val = 0;
	Assert(!Parse<char>::ParseHexInteger(p, val));
}
