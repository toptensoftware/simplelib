#include "../UnitTesting.h"
#include "../Core.h"
using namespace SimpleLib;

Fact("StringCore Construction")
{
	// Basic constructor
	String str("Hello");
	Assert(str.IsEqualTo("Hello"));
	Assert(str.GetLength()==5);
}

Fact("StringCore IndexOf")
{
	String str("Hello");
	Assert(str.IndexOfAny("l") == 2);
	Assert(str.IndexOfAny("fl") == 2);
	Assert(str.IndexOfAny<SCaseI>("fL") == 2);
	Assert(str.LastIndexOfAny("l") == 3);
	Assert(str.LastIndexOfAny("fl") == 3);
	Assert(str.LastIndexOfAny<SCaseI>("fL") == 3);
}

Fact("StringCore Copy Constructor")
{
	// Copy constructor
	String str("Hello");
	String str2(str);
	Assert(static_cast<const char*>(str)==static_cast<const char*>(str2));			// Pointers should be same
}

Fact("StringCore Character Access")
{
	// Character access
	String str("Hello");
	Assert(str[0]=='H');
	Assert(str[4]=='o');
}


Fact("StringCore Length specified constructor")
{
	// Length specified constructor
	String str=String("Hello World", 5);
	Assert(str.IsEqualTo("Hello"));
}

Fact("Sub StringCore")
{
	String strA("Hello World");
	Assert(strA.SubString(0, 5).IsEqualTo("Hello"));
	Assert(strA.SubString(6, 5).IsEqualTo("World"));
	Assert(strA.ToUpper().IsEqualTo("HELLO WORLD"));
	Assert(strA.ToLower().IsEqualTo("hello world"));
	Assert(strA.IsEqualTo("Hello World"));

	WString strW(L"Hello World");
	Assert(strW.SubString(0, 5).IsEqualTo(L"Hello"));
	Assert(strW.SubString(6, 5).IsEqualTo(L"World"));
	Assert(strW.ToUpper().IsEqualTo(L"HELLO WORLD"));
	Assert(strW.ToLower().IsEqualTo(L"hello world"));
	Assert(strW.IsEqualTo(L"Hello World"));
}

Fact("StringCore CaseI Compare")
{
	Assert(SCaseI::Compare("Hello World", "hello world")==0);
	Assert(SCaseI::Compare(L"Hello World", L"hello world")==0);
}

Fact("StringCore Starts/Ends With")
{
	String strA("Hello World");
	Assert(strA.StartsWith("Hello"));
	Assert(strA.EndsWith("World"));
	Assert(strA.StartsWith<SCaseI>("HELLO"));
	Assert(strA.EndsWith<SCaseI>("WORLD"));

	WString strW(L"Hello World");
	Assert(strW.StartsWith(L"Hello"));
	Assert(strW.EndsWith(L"World"));
	Assert(strW.StartsWith<SCaseI>(L"HELLO"));
	Assert(strW.EndsWith<SCaseI>(L"WORLD"));
}

Fact("StringCore Split")
{
	String strA = "Apples;Pears;;Bananas";

	List<String> parts;
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

Fact("StringCore Replace")
{
	Assert(String("Apples Pears Bananas").Replace("Apples", "Oranges").IsEqualTo("Oranges Pears Bananas"));
	Assert(String("Apples Pears Bananas").Replace("Pears", "Oranges").IsEqualTo("Apples Oranges Bananas"));
	Assert(String("Apples Pears Bananas").Replace("Bananas", "Oranges").IsEqualTo("Apples Pears Oranges"));
	Assert(String("Apples Pears Bananas").Replace<SCaseI>("PEARS", "Oranges").IsEqualTo("Apples Oranges Bananas"));
}

Fact("StringCore Replace Wide Char")
{
	Assert(WString(L"Apples Pears Bananas").Replace(L"Pears", L"Oranges").IsEqualTo(L"Apples Oranges Bananas"));
	Assert(WString(L"Apples Pears Bananas").Replace<SCaseI>(L"PEARS", L"Oranges").IsEqualTo(L"Apples Oranges Bananas"));
}

Fact("StringCore Default Constructor")
{
	String str;
	Assert(str.IsNull());
	Assert(str.IsEmpty());
	Assert(str.GetLength() == 0);
	Assert(str.sz() == nullptr);
}

Fact("StringCore Null Pointer Constructor")
{
	String str(nullptr);
	Assert(str.IsNull());
	Assert(str.IsEmpty());
	Assert(str.sz() == nullptr);
}

Fact("StringCore Move Constructor")
{
	String a("Hello");
	String b(move(a));
	Assert(b.IsEqualTo("Hello"));
	Assert(a.IsNull());
}

Fact("StringCore Move Assignment")
{
	String a("Hello");
	String b("World");
	b = move(a);
	Assert(b.IsEqualTo("Hello"));
	Assert(a.IsNull());
}

Fact("StringCore Copy Assignment")
{
	String a("Hello");
	String b;
	b = a;
	Assert(b.IsEqualTo("Hello"));
	Assert(static_cast<const char*>(a) == static_cast<const char*>(b));	// Pointers should be same (ref-counted)
}

Fact("StringCore Assign From Literal")
{
	String str;
	str = "Hello";
	Assert(str.IsEqualTo("Hello"));

	str.Assign("Hello World", 5);
	Assert(str.IsEqualTo("Hello"));
}

Fact("StringCore Clear")
{
	String str("Hello");
	str.Clear();
	Assert(str.IsNull());
	Assert(str.IsEmpty());
	Assert(str.GetLength() == 0);
}

Fact("StringCore Equality Operator")
{
	String a("Hello");
	String b("Hello");
	String c("World");
	Assert(a == b);
	Assert(!(a == c));
}

Fact("StringCore Inequality Operator")
{
	String a("Hello");
	String b("Hello");
	String c("World");
	Assert(a != c);
	Assert(!(a != b));
}

Fact("StringCore Less Than / Greater Than Operators")
{
	String a("Apple");
	String b("Banana");
	String c("Apple");

	Assert(a < b);
	Assert(!(b < a));
	Assert(!(a < c));

	Assert(b > a);
	Assert(!(a > b));
	Assert(!(a > c));
}

Fact("StringCore Less Than Or Equal / Greater Than Or Equal Operators")
{
	String a("Apple");
	String b("Banana");
	String c("Apple");

	Assert(a <= b);
	Assert(a <= c);
	Assert(!(b <= a));

	Assert(b >= a);
	Assert(a >= c);
	Assert(!(a >= b));
}

Fact("StringCore Not Operator")
{
	String empty;
	String emptyStr("");
	String nonEmpty("Hello");

	Assert(!empty);
	Assert(!emptyStr);
	Assert(!nonEmpty == false);
}

Fact("StringCore Concatenation Operator+")
{
	String a("Hello");
	String b(" World");
	String c = a + b;
	Assert(c.IsEqualTo("Hello World"));

	// Operands unaffected
	Assert(a.IsEqualTo("Hello"));
	Assert(b.IsEqualTo(" World"));
}

Fact("StringCore Concatenation Operator+=")
{
	String a("Hello");
	a += " World";
	Assert(a.IsEqualTo("Hello World"));

	String b("Foo");
	b += String("Bar");
	Assert(b.IsEqualTo("FooBar"));
}

Fact("StringCore Const Char Pointer Conversion")
{
	String str("Hello");
	const char* psz = str;
	Assert(strcmp(psz, "Hello") == 0);
}

Fact("StringCore IndexOf Char")
{
	String str("Hello");
	Assert(str.IndexOf('l') == 2);
	Assert(str.IndexOf('l', 3) == 3);
	Assert(str.IndexOf('z') == -1);
}

Fact("StringCore IndexOf Substring")
{
	String str("Hello World Hello");
	Assert(str.IndexOf("World") == 6);
	Assert(str.IndexOf("Hello") == 0);
	Assert(str.IndexOf("Hello", 1) == 12);
	Assert(str.IndexOf("xyz") == -1);
	Assert(str.IndexOf<SCaseI>("WORLD") == 6);
}

Fact("StringCore LastIndexOf Substring")
{
	String str("abc abc abc");
	Assert(str.LastIndexOf("abc") == 8);
	Assert(str.LastIndexOf("abc", 7) == 4);
	Assert(str.LastIndexOf("xyz") == -1);
	Assert(str.LastIndexOf<SCaseI>("ABC") == 8);
}

Fact("StringCore IsEqualTo Case Insensitive")
{
	String str("Hello");
	Assert(!str.IsEqualTo("HELLO"));
	Assert(str.IsEqualTo<SCaseI>("HELLO"));
}

Fact("StringCore IsNullOrEmpty")
{
	Assert(String::IsNullOrEmpty(nullptr));
	Assert(String::IsNullOrEmpty(""));
	Assert(!String::IsNullOrEmpty("x"));
}

Fact("StringCore Join")
{
	List<String> parts;
	parts.Add("Apples");
	parts.Add("Pears");
	parts.Add("Bananas");
	Assert(String::Join(parts, ';').IsEqualTo("Apples;Pears;Bananas"));
}

Fact("StringCore Format Integers")
{
	Assert(String::Format("%d", 42).IsEqualTo("42"));
	Assert(String::Format("%d", -42).IsEqualTo("-42"));
	Assert(String::Format("%5d", 42).IsEqualTo("   42"));
	Assert(String::Format("%-5d|", 42).IsEqualTo("42   |"));
	Assert(String::Format("%05d", 42).IsEqualTo("00042"));
}

Fact("StringCore Format Strings And Chars")
{
	Assert(String::Format("%s", "hi").IsEqualTo("hi"));
	Assert(String::Format("[%5s]", "hi").IsEqualTo("[   hi]"));
	Assert(String::Format("[%-5s]", "hi").IsEqualTo("[hi   ]"));
	Assert(String::Format("%c", 'A').IsEqualTo("A"));
	Assert(String::Format("100%%").IsEqualTo("100%"));
}

Fact("StringCore Format Hex And Float")
{
	Assert(String::Format("%x", 255).IsEqualTo("ff"));
	Assert(String::Format("%X", 255).IsEqualTo("FF"));
	Assert(String::Format("%.2f", 3.14159).IsEqualTo("3.14"));
}

Fact("StringCore CopyToBuffer")
{
	String str("Hello");
	char buf[10];
	Assert(str.CopyToBuffer(buf, 10));
	Assert(strcmp(buf, "Hello") == 0);

	char smallBuf[3];
	Assert(!str.CopyToBuffer(smallBuf, 3));
}

Fact("StringCore AllocCopy")
{
	String str("Hello");
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

Fact("StringCore Hash")
{
	Assert(String::Hash(String("Hello")) == String::Hash(String("Hello")));
	Assert(String::Hash(String("Hello")) != String::Hash(String("World")));
}