#include "../UnitTesting.h"
#include "../Core.h"
using namespace SimpleLib;

Fact("String Construction")
{
	// Basic constructor
	String<char> str("Hello");
	Assert(str.IsEqualTo("Hello"));
	Assert(str.GetLength()==5);
}

Fact("String IndexOf")
{
	String<char> str("Hello");
	Assert(str.IndexOfAny("l") == 2);
	Assert(str.IndexOfAny("fl") == 2);
	Assert(str.IndexOfAny<SCaseI>("fL") == 2);
	Assert(str.LastIndexOfAny("l") == 3);
	Assert(str.LastIndexOfAny("fl") == 3);
	Assert(str.LastIndexOfAny<SCaseI>("fL") == 3);
}

Fact("String Copy Constructor")
{
	// Copy constructor
	String<char> str("Hello");
	String<char> str2(str);
	Assert(static_cast<const char*>(str)==static_cast<const char*>(str2));			// Pointers should be same
}

Fact("String Character Access")
{
	// Character access
	String<char> str("Hello");
	Assert(str[0]=='H');
	Assert(str[4]=='o');
}


Fact("String Length specified constructor")
{
	// Length specified constructor
	String<char> str=String<char>("Hello World", 5);
	Assert(str.IsEqualTo("Hello"));
}

Fact("Sub String")
{
	String<char> strA("Hello World");
	Assert(strA.SubString(0, 5).IsEqualTo("Hello"));
	Assert(strA.SubString(6, 5).IsEqualTo("World"));
	Assert(strA.ToUpper().IsEqualTo("HELLO WORLD"));
	Assert(strA.ToLower().IsEqualTo("hello world"));
	Assert(strA.IsEqualTo("Hello World"));

	String<wchar_t> strW(L"Hello World");
	Assert(strW.SubString(0, 5).IsEqualTo(L"Hello"));
	Assert(strW.SubString(6, 5).IsEqualTo(L"World"));
	Assert(strW.ToUpper().IsEqualTo(L"HELLO WORLD"));
	Assert(strW.ToLower().IsEqualTo(L"hello world"));
	Assert(strW.IsEqualTo(L"Hello World"));
}

Fact("String CaseI Compare")
{
	Assert(SCaseI::Compare("Hello World", "hello world")==0);
	Assert(SCaseI::Compare(L"Hello World", L"hello world")==0);
}

Fact("String Starts/Ends With")
{
	String<char> strA("Hello World");
	Assert(strA.StartsWith("Hello"));
	Assert(strA.EndsWith("World"));
	Assert(strA.StartsWith<SCaseI>("HELLO"));
	Assert(strA.EndsWith<SCaseI>("WORLD"));

	String<wchar_t> strW(L"Hello World");
	Assert(strW.StartsWith(L"Hello"));
	Assert(strW.EndsWith(L"World"));
	Assert(strW.StartsWith<SCaseI>(L"HELLO"));
	Assert(strW.EndsWith<SCaseI>(L"WORLD"));
}

Fact("String Split")
{
	String<char> strA = "Apples;Pears;;Bananas";

	List<String<char>> parts;
	strA.Split(";", true, parts);
	Assert(parts.GetCount() == 4);
	Assert(parts[0].IsEqualTo("Apples"));
	Assert(parts[1].IsEqualTo("Pears"));
	Assert(parts[2].IsEmpty());
	Assert(parts[3].IsEqualTo("Bananas"));

	parts.Clear();
	strA.Split(";", false, parts);
	Assert(parts.GetCount() == 3);
	Assert(parts[0].IsEqualTo("Apples"));
	Assert(parts[1].IsEqualTo("Pears"));
	Assert(parts[2].IsEqualTo("Bananas"));
}

Fact("String Replace")
{
	Assert(String<char>("Apples Pears Bananas").Replace("Apples", "Oranges").IsEqualTo("Oranges Pears Bananas"));
	Assert(String<char>("Apples Pears Bananas").Replace("Pears", "Oranges").IsEqualTo("Apples Oranges Bananas"));
	Assert(String<char>("Apples Pears Bananas").Replace("Bananas", "Oranges").IsEqualTo("Apples Pears Oranges"));
	Assert(String<char>("Apples Pears Bananas").Replace<SCaseI>("PEARS", "Oranges").IsEqualTo("Apples Oranges Bananas"));
}

Fact("String Replace Wide Char")
{
	Assert(String<wchar_t>(L"Apples Pears Bananas").Replace(L"Pears", L"Oranges").IsEqualTo(L"Apples Oranges Bananas"));
	Assert(String<wchar_t>(L"Apples Pears Bananas").Replace<SCaseI>(L"PEARS", L"Oranges").IsEqualTo(L"Apples Oranges Bananas"));
}

Fact("String Default Constructor")
{
	String<char> str;
	Assert(str.IsNull());
	Assert(str.IsEmpty());
	Assert(str.GetLength() == 0);
	Assert(str.sz() == nullptr);
}

Fact("String Null Pointer Constructor")
{
	String<char> str(nullptr);
	Assert(str.IsNull());
	Assert(str.IsEmpty());
	Assert(str.sz() == nullptr);
}

Fact("String Move Constructor")
{
	String<char> a("Hello");
	String<char> b(move(a));
	Assert(b.IsEqualTo("Hello"));
	Assert(a.IsNull());
}

Fact("String Move Assignment")
{
	String<char> a("Hello");
	String<char> b("World");
	b = move(a);
	Assert(b.IsEqualTo("Hello"));
	Assert(a.IsNull());
}

Fact("String Copy Assignment")
{
	String<char> a("Hello");
	String<char> b;
	b = a;
	Assert(b.IsEqualTo("Hello"));
	Assert(static_cast<const char*>(a) == static_cast<const char*>(b));	// Pointers should be same (ref-counted)
}

Fact("String Assign From Literal")
{
	String<char> str;
	str = "Hello";
	Assert(str.IsEqualTo("Hello"));

	str.Assign("Hello World", 5);
	Assert(str.IsEqualTo("Hello"));
}

Fact("String Clear")
{
	String<char> str("Hello");
	str.Clear();
	Assert(str.IsNull());
	Assert(str.IsEmpty());
	Assert(str.GetLength() == 0);
}

Fact("String Equality Operator")
{
	String<char> a("Hello");
	String<char> b("Hello");
	String<char> c("World");
	Assert(a == b);
	Assert(!(a == c));
}

Fact("String Concatenation Operator+")
{
	String<char> a("Hello");
	String<char> b(" World");
	String<char> c = a + b;
	Assert(c.IsEqualTo("Hello World"));

	// Operands unaffected
	Assert(a.IsEqualTo("Hello"));
	Assert(b.IsEqualTo(" World"));
}

Fact("String Concatenation Operator+=")
{
	String<char> a("Hello");
	a += " World";
	Assert(a.IsEqualTo("Hello World"));

	String<char> b("Foo");
	b += String<char>("Bar");
	Assert(b.IsEqualTo("FooBar"));
}

Fact("String Const Char Pointer Conversion")
{
	String<char> str("Hello");
	const char* psz = str;
	Assert(strcmp(psz, "Hello") == 0);
}

Fact("String IndexOf Char")
{
	String<char> str("Hello");
	Assert(str.IndexOf('l') == 2);
	Assert(str.IndexOf('l', 3) == 3);
	Assert(str.IndexOf('z') == -1);
}

Fact("String IndexOf Substring")
{
	String<char> str("Hello World Hello");
	Assert(str.IndexOf("World") == 6);
	Assert(str.IndexOf("Hello") == 0);
	Assert(str.IndexOf("Hello", 1) == 12);
	Assert(str.IndexOf("xyz") == -1);
	Assert(str.IndexOf<SCaseI>("WORLD") == 6);
}

Fact("String LastIndexOf Substring")
{
	String<char> str("abc abc abc");
	Assert(str.LastIndexOf("abc") == 8);
	Assert(str.LastIndexOf("abc", 7) == 4);
	Assert(str.LastIndexOf("xyz") == -1);
	Assert(str.LastIndexOf<SCaseI>("ABC") == 8);
}

Fact("String IsEqualTo Case Insensitive")
{
	String<char> str("Hello");
	Assert(!str.IsEqualTo("HELLO"));
	Assert(str.IsEqualTo<SCaseI>("HELLO"));
}

Fact("String IsNullOrEmpty")
{
	Assert(String<char>::IsNullOrEmpty(nullptr));
	Assert(String<char>::IsNullOrEmpty(""));
	Assert(!String<char>::IsNullOrEmpty("x"));
}

Fact("String Join")
{
	List<String<char>> parts;
	parts.Add("Apples");
	parts.Add("Pears");
	parts.Add("Bananas");
	Assert(String<char>::Join(parts, ';').IsEqualTo("Apples;Pears;Bananas"));
}

Fact("String Format Integers")
{
	Assert(String<char>::Format("%d", 42).IsEqualTo("42"));
	Assert(String<char>::Format("%d", -42).IsEqualTo("-42"));
	Assert(String<char>::Format("%5d", 42).IsEqualTo("   42"));
	Assert(String<char>::Format("%-5d|", 42).IsEqualTo("42   |"));
	Assert(String<char>::Format("%05d", 42).IsEqualTo("00042"));
}

Fact("String Format Strings And Chars")
{
	Assert(String<char>::Format("%s", "hi").IsEqualTo("hi"));
	Assert(String<char>::Format("[%5s]", "hi").IsEqualTo("[   hi]"));
	Assert(String<char>::Format("[%-5s]", "hi").IsEqualTo("[hi   ]"));
	Assert(String<char>::Format("%c", 'A').IsEqualTo("A"));
	Assert(String<char>::Format("100%%").IsEqualTo("100%"));
}

Fact("String Format Hex And Float")
{
	Assert(String<char>::Format("%x", 255).IsEqualTo("ff"));
	Assert(String<char>::Format("%X", 255).IsEqualTo("FF"));
	Assert(String<char>::Format("%.2f", 3.14159).IsEqualTo("3.14"));
}

Fact("String CopyToBuffer")
{
	String<char> str("Hello");
	char buf[10];
	Assert(str.CopyToBuffer(buf, 10));
	Assert(strcmp(buf, "Hello") == 0);

	char smallBuf[3];
	Assert(!str.CopyToBuffer(smallBuf, 3));
}

Fact("String AllocCopy")
{
	String<char> str("Hello");
	char* copy = str.AllocCopy();
	Assert(strcmp(copy, "Hello") == 0);
	free(copy);

	// Buffer too small for required length
	Assert(str.AllocCopy(3) == nullptr);

	// Buffer exactly large enough
	char* copy2 = str.AllocCopy(6);
	Assert(copy2 != nullptr);
	Assert(strcmp(copy2, "Hello") == 0);
	free(copy2);
}

Fact("String Hash")
{
	Assert(String<char>::Hash(String<char>("Hello")) == String<char>::Hash(String<char>("Hello")));
	Assert(String<char>::Hash(String<char>("Hello")) != String<char>::Hash(String<char>("World")));
}