#include "../UnitTesting.h"
#include "../Core.h"
using namespace SimpleLib;

// Shared test data: the string "A" + LATIN SMALL LETTER E WITH ACUTE (U+00E9)
// + EURO SIGN (U+20AC) + GRINNING FACE emoji (U+1F600), expressed in each
// encoding, so every conversion can be checked against a single reference.
// Every value is written as an explicit numeric escape (\xNN / \uNNNN /
// \UNNNNNNNN) so the tests don't depend on the source file's encoding.
//   U+0041   'A'            -> 1 UTF-8 byte,  1 UTF-16 unit
//   U+00E9   e-acute        -> 2 UTF-8 bytes, 1 UTF-16 unit
//   U+20AC   Euro sign      -> 3 UTF-8 bytes, 1 UTF-16 unit
//   U+1F600  Grinning face  -> 4 UTF-8 bytes, 2 UTF-16 units (surrogate pair)
static const char* s_utf8 = "A\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80";
static const char32_t* s_utf32 = U"A\u00E9\u20AC\U0001F600";
static const char16_t* s_utf16 = u"A\u00E9\u20AC\U0001F600";
static const wchar_t* s_wide = L"A\u00E9\u20AC\U0001F600";

Fact("Encoding Passthrough")
{
	Assert(Encode<char>("Hello").IsEqualTo("Hello"));
	Assert(Encode<wchar_t>(L"Hello").IsEqualTo(L"Hello"));
	Assert(Encode<char16_t>(u"Hello").IsEqualTo(u"Hello"));
	Assert(Encode<char32_t>(U"Hello").IsEqualTo(U"Hello"));
}

Fact("Encoding Null Input")
{
	Assert(Encode<char>((const wchar_t*)nullptr).IsNull());
	Assert(Encode<wchar_t>((const char*)nullptr).IsNull());
	Assert(Encode<char32_t>((const char*)nullptr).IsNull());
}

Fact("Encoding Empty Input")
{
	Assert(Encode<char32_t>("").IsEmpty());
	Assert(Encode<char>(U"").IsEmpty());
}

Fact("Encoding UTF8 To UTF32")
{
	StringCore<char32_t> result = Encode<char32_t>(s_utf8);
	Assert(result.GetLength() == 4);
	Assert(result.IsEqualTo(s_utf32));
}

Fact("Encoding UTF32 To UTF8")
{
	String result = Encode<char>(s_utf32);
	Assert(result.IsEqualTo(s_utf8));
}

Fact("Encoding UTF32 To UTF16")
{
	StringCore<char16_t> result = Encode<char16_t>(s_utf32);
	Assert(result.GetLength() == 5);	// 1+1+1+2 (surrogate pair for U+1F600)
	Assert(result.IsEqualTo(s_utf16));
}

Fact("Encoding UTF16 To UTF32")
{
	StringCore<char32_t> result = Encode<char32_t>(s_utf16);
	Assert(result.GetLength() == 4);
	Assert(result.IsEqualTo(s_utf32));
}

Fact("Encoding UTF8 To UTF16")
{
	StringCore<char16_t> result = Encode<char16_t>(s_utf8);
	Assert(result.IsEqualTo(s_utf16));
}

Fact("Encoding UTF16 To UTF8")
{
	String result = Encode<char>(s_utf16);
	Assert(result.IsEqualTo(s_utf8));
}

Fact("Encoding UTF8 To WChar")
{
	WString result = Encode<wchar_t>(s_utf8);
	Assert(result.IsEqualTo(s_wide));
}

Fact("Encoding WChar To UTF8")
{
	String result = Encode<char>(s_wide);
	Assert(result.IsEqualTo(s_utf8));
}

Fact("Encoding Round Trip UTF8 Via All Forms")
{
	// UTF-8 -> UTF-32 -> UTF-16 -> UTF-8
	StringCore<char32_t> asUtf32 = Encode<char32_t>(s_utf8);
	StringCore<char16_t> asUtf16 = Encode<char16_t>(asUtf32.sz());
	String backToUtf8 = Encode<char>(asUtf16.sz());
	Assert(backToUtf8.IsEqualTo(s_utf8));

	// UTF-8 -> WChar -> UTF-8
	WString asWide = Encode<wchar_t>(s_utf8);
	String backToUtf8b = Encode<char>(asWide.sz());
	Assert(backToUtf8b.IsEqualTo(s_utf8));
}

Fact("Encoding ASCII Only Fast Path")
{
	// Every stage should handle plain ASCII identically to a plain copy
	Assert(Encode<char32_t>("Hello World").IsEqualTo(U"Hello World"));
	Assert(Encode<char16_t>("Hello World").IsEqualTo(u"Hello World"));
	Assert(Encode<char>(U"Hello World").IsEqualTo("Hello World"));
	Assert(Encode<char>(u"Hello World").IsEqualTo("Hello World"));
}
