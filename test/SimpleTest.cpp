// SimpleTest.cpp : SimpleLib Unit Tests


#define SIMPLELIB_POSIX_PATHS
#include "../SimpleLib.h"

using namespace SimpleLib;

bool g_bAnyFailed=false;
bool g_bFailed=false;

typedef String<char> CAnsiString;
typedef String<wchar_t> CUniString;

void Failed(int iLine, const char* psz)
{
	if (!g_bFailed) printf("\n");
	g_bAnyFailed=true;
	g_bFailed=true;
	printf("Failed(%i): %s\n", iLine, psz);
}

#undef assert
#define assert(x)  if (!(x)) Failed(__LINE__, #x);

class InstanceCounter
{
public:
	InstanceCounter()
	{
		m_iInstances++;
	}

	InstanceCounter(const InstanceCounter& Other)
	{
		m_iInstances++;
	}

	virtual ~InstanceCounter()
	{
		m_iInstances--;
	}

	static int m_iInstances;
};

int InstanceCounter::m_iInstances=0;

class MyObject
{
public:
	MyObject(int iVal) : m_iVal(iVal)
	{
	}
	MyObject(const MyObject& Other) : m_iVal(Other.m_iVal)
	{
	}

	int m_iVal;
};

void TestMath()
{
	printf("Testing Math...");
	g_bFailed=false;

	assert(Math::Min(10, 20) == 10);
	assert(Math::Max(10, 20) == 20);

	if (!g_bFailed)
		printf("OK\n");
}

void TestVector()
{
	printf("Testing Vector...");

	VectorI a(10, 10);
	VectorI b(10, 10);
	VectorI c(1,2);
	assert(a == b);
	assert(a != c);

	auto x = VectorD(0, 10).Magnitude();
	auto y = VectorD::Distance(VectorD(10, 10), VectorD(20, 20));

	if (!g_bFailed)
		printf("OK\n");
}

void TestRectangle()
{
	printf("Testing Rectangle...");

	assert(RectangleD(10, 10, 20, 20).Area() == 400.0);
	assert(RectangleF(10, 10, 20, 20).Area() == 400.0F);
	assert(RectangleI(10, 10, 20, 20).Area() == 400);

	RectangleI a(10, 10, 20, 20);
	RectangleI b(10, 10, 20, 20);
	RectangleI c(1,2,3,4);
	assert(a == b);
	assert(a != c);

	if (!g_bFailed)
		printf("OK\n");
}

void TestStrings()
{
	g_bFailed=false;
	printf("Testing String...");


	// Basic constructor
	CAnsiString str("Hello");
	assert(str.IsEqualTo("Hello"));
	assert(str.GetLength()==5);

	assert(str.IndexOfAny("l") == 2);
	assert(str.IndexOfAny("fl") == 2);
	assert(str.IndexOfAny<SCaseI>("fL") == 2);

	assert(str.LastIndexOfAny("l") == 3);
	assert(str.LastIndexOfAny("fl") == 3);
	assert(str.LastIndexOfAny<SCaseI>("fL") == 3);

	// Copy constructor
	CAnsiString str2(str);
	assert(static_cast<const char*>(str)==static_cast<const char*>(str2));			// Pointers should be same

	// Character access
	assert(str[0]=='H');
	assert(str[4]=='o');

	// Length specified constructor
	str2=CAnsiString("Hello World", 5);
	assert(str2.IsEqualTo("Hello"));

	CAnsiString strA("Hello World");
	assert(strA.SubString(0, 5).IsEqualTo("Hello"));
	assert(strA.SubString(6, 5).IsEqualTo("World"));
	assert(strA.ToUpper().IsEqualTo("HELLO WORLD"));
	assert(strA.ToLower().IsEqualTo("hello world"));
	assert(strA.IsEqualTo("Hello World"));

	CUniString strW(L"Hello World");
	assert(strW.SubString(0, 5).IsEqualTo(L"Hello"));
	assert(strW.SubString(6, 5).IsEqualTo(L"World"));
	assert(strW.ToUpper().IsEqualTo(L"HELLO WORLD"));
	assert(strW.ToLower().IsEqualTo(L"hello world"));
	assert(strW.IsEqualTo(L"Hello World"));

	assert(SCaseI::Compare("Hello World", "hello world")==0);
	assert(SCaseI::Compare(L"Hello World", L"hello world")==0);

	assert(strA.StartsWith("Hello"));
	assert(strA.EndsWith("World"));
	assert(strA.StartsWith<SCaseI>("HELLO"));
	assert(strA.EndsWith<SCaseI>("WORLD"));

	assert(strW.StartsWith(L"Hello"));
	assert(strW.EndsWith(L"World"));
	assert(strW.StartsWith<SCaseI>(L"HELLO"));
	assert(strW.EndsWith<SCaseI>(L"WORLD"));

	strA = "Apples;Pears;;Bananas";

	List<CAnsiString> parts;
	strA.Split(";", true, parts);
	assert(parts.GetCount() == 4);
	assert(parts[0].IsEqualTo("Apples"));
	assert(parts[1].IsEqualTo("Pears"));
	assert(parts[2].IsEmpty());
	assert(parts[3].IsEqualTo("Bananas"));

	parts.Clear();
	strA.Split(";", false, parts);
	assert(parts.GetCount() == 3);
	assert(parts[0].IsEqualTo("Apples"));
	assert(parts[1].IsEqualTo("Pears"));
	assert(parts[2].IsEqualTo("Bananas"));

	assert(CAnsiString("Apples Pears Bananas").Replace("Apples", "Oranges").IsEqualTo("Oranges Pears Bananas"));
	assert(CAnsiString("Apples Pears Bananas").Replace("Pears", "Oranges").IsEqualTo("Apples Oranges Bananas"));
	assert(CAnsiString("Apples Pears Bananas").Replace("Bananas", "Oranges").IsEqualTo("Apples Pears Oranges"));
	assert(CAnsiString("Apples Pears Bananas").Replace<SCaseI>("PEARS", "Oranges").IsEqualTo("Apples Oranges Bananas"));

	if (!g_bFailed)
		printf("OK\n");
}

void TestList()
{
	g_bFailed=false;
	printf("Testing List...");

	// Setup a list
	List<int> list;

	list.Add(10);
	assert(list.IndexOf(10) == 0);
	list.Clear();

	// Add 10 items
	int i;
	for (i=0; i<10; i++)
	{
		list.Add(i);
	}

	// Test initial addd
	assert(list.GetCount()==10);
	for (i=0; i<10; i++)
	{
		assert(list[i]==i);
	}

	// Insert At
	list.InsertAt(5, 100);
	assert(list[4]==4);
	assert(list[5]==100);
	assert(list[6]==5);

	// Remove At
	list.RemoveAt(5);
	assert(list[4]==4);
	assert(list[5]==5);

	// Remove At (multiple)
	list.InsertAt(5, 100);
	list.InsertAt(5, 100);
	list.InsertAt(5, 100);
	list.InsertAt(5, 100);
	list.RemoveAt(5, 4);
	assert(list[4]==4);
	assert(list[5]==5);

	// Swap
	list.Swap(1,2);
	assert(list[0]==0);
	assert(list[1]==2);
	assert(list[2]==1);
	assert(list[3]==3);
	list.Swap(1,2);

	// Move
	list.Move(3,1);
	assert(list[0]==0);
	assert(list[1]==3);
	assert(list[2]==1);
	assert(list[3]==2);

	// Move back
	list.Move(1,3);
	assert(list[0]==0);
	assert(list[1]==1);
	assert(list[2]==2);
	assert(list[3]==3);

	// Find
	assert(list.IndexOf(100)==-1);
	assert(list.IndexOf(8)==8);

	// Find with start position
	list.Add(1);
	assert(list.IndexOf(1)==1);
	assert(list.IndexOf(1,5)==10);
	assert(!list.IsEmpty());

	// Remove All
	list.Clear();
	assert(list.GetCount()==0);
	assert(list.IsEmpty());

	// Test stack operations
	list.Clear();
	list.Push(10);
	list.Push(20);
	list.Push(30);
	assert(list[0]==10);
	assert(list[1]==20);
	assert(list[2]==30);
	assert(list.Tail()==30);
	assert(list.Pop()==30);
	assert(list.Pop()==20);
	assert(list.Pop()==10);
	assert(list.IsEmpty());

	// Test queue operations
	list.Clear();
	list.Enqueue(10);
	list.Enqueue(20);
	list.Enqueue(30);
	assert(list[0]==10);
	assert(list[1]==20);
	assert(list[2]==30);
	assert(list.Head()==10);
	assert(list.Dequeue()==10);
	assert(list.Dequeue()==20);
	assert(list.Dequeue()==30);
	assert(list.IsEmpty());

	// Test construction/destruction when holding objects
	List<InstanceCounter>	vecObjs;
	{
	vecObjs.Add(InstanceCounter());
	vecObjs.Add(InstanceCounter());
	vecObjs.Add(InstanceCounter());
	}
	assert(InstanceCounter::m_iInstances==3);
	vecObjs.RemoveAt(0);
	assert(InstanceCounter::m_iInstances==2);
	vecObjs.Clear();
	assert(InstanceCounter::m_iInstances==0);


	// Test construction/destruction when holding owned object pointers
	List<SharedPtr<InstanceCounter>> vecPtrs;
	vecPtrs.Add(new InstanceCounter());
	vecPtrs.Add(new InstanceCounter());
	vecPtrs.Add(new InstanceCounter());
	assert(InstanceCounter::m_iInstances==3);
	vecPtrs.RemoveAt(0);
	assert(InstanceCounter::m_iInstances==2);
	vecPtrs.Clear();
	assert(InstanceCounter::m_iInstances==0);

	List<String<wchar_t>> strs;
	strs.Add(L"Apples");
	strs.Add(L"Pears");
	strs.Add(L"Bananas");
	assert(strs.IndexOf(L"Pears") == 1);
	assert(strs.IndexOf<SCaseI>(L"PEARS") == 1);


	if (!g_bFailed)
		printf("OK\n");
}

void TestMap()
{
	printf("Testing Dictionary...");
	g_bFailed=false;

	// Start with empty map
	Dictionary<int, int> map;
	assert(map.IsEmpty());
	assert(map.GetCount()==0);

	// Add some items
	int i;
	for (i=1; i<=100; i++)
	{
		map.Add(i, i*10);
	}
	assert(!map.IsEmpty());
	assert(map.GetCount()==100);
#ifdef _DEBUG
	map.CheckAll();
#endif


	// Check lookups...
	assert(map.Get(1)==10);
	assert(map.Get(5)==50);
	assert(map.Get(10)==100);
	assert(map.Get(2000, -1)==-1);
	assert(map.ContainsKey(1));
	assert(map.ContainsKey(50));
	assert(map.ContainsKey(100));
	assert(!map.ContainsKey(0));
	assert(!map.ContainsKey(101));

	// Check find  (Get above uses Find so just one test...)
	int iValue;
	assert(map.TryGetValue(1, iValue)==true && iValue==10);

	// Test iteration
	for (i=0; i<map.GetCount(); i++)
	{
		assert(map.GetAt(i).Key==i+1);
		assert(map.GetAt(i).Value==map.GetAt(i).Key*10);
	}
#ifdef _DEBUG
	map.CheckAll();
#endif

	// Test iteration while removing... remove all odd elements
	// NB: Behaviour when forward iterating and removing should be same as for
	//		vector, where when removing current element, iteration index should be decremented
	int iTotal=map.GetCount();
	int iIndex=0;
	for (i=0; i<map.GetCount(); i++, iIndex++)
	{
		int iKey=map.GetAt(i).Key;
		assert(iKey==iIndex+1);

		if ((iKey%2)!=0)
		{
			map.Remove(map.GetAt(i).Key);
			iTotal--;
			i--;
		}
	}
#ifdef _DEBUG
	map.CheckAll();
#endif
	assert(map.GetCount()==iTotal);

	// Check only evens left...
	assert(map.GetCount()==50);
	for (i=0; i<map.GetCount(); i++)
	{
		assert((map.GetAt(i).Key%2)==0);
	}

	// Test reverse iteration while removing
	for (i=map.GetCount()-1; i>=0; i--)
	{
		if ((map.GetAt(i).Key % 10)==0)
		{
			map.Remove(map.GetAt(i).Key);
			iTotal--;
		}
	}
#ifdef _DEBUG
	map.CheckAll();
#endif
	assert(map.GetCount()==iTotal);

	// Repopulate...
	map.Clear();
	assert(map.GetCount()==0);
	assert(map.IsEmpty());
	for (i=0; i<100; i++)
	{
		map.Add(i*10, i*10);
	}

//	srand(0);

	// Test insertion while iterating...
	int iExpectedLoopCount=map.GetCount();
	int iLoopCount=0;
	int iPrevKey=0;
	for (i=0; i<map.GetCount(); i++)
	{
		int iKey=map.GetAt(i).Key;

		if (i>0)
		{
			assert(iKey>iPrevKey);
		}
		iPrevKey=iKey;


		assert(map.GetAt(i).Key==map.GetAt(i).Value);

		iLoopCount++;

		int iNew=rand()%1000;
		if (iNew<=iKey)
		{
			if (!map.ContainsKey(iNew))
				i++;
		}

		if (iNew>iKey)
		{
			if (!map.ContainsKey(iNew))
				iExpectedLoopCount++;
		}

		map.Set(iNew, iNew);
	}
	assert(iExpectedLoopCount==iLoopCount);


	// Test construction/destruction when holding objects
	// NB: There's an extra 1 on the reference count because of the instance in
	//		the m_Leaf member of the map itself.
	Dictionary<int, InstanceCounter>	mapObjs;
	{ mapObjs.Add(10, InstanceCounter()); }
	assert(InstanceCounter::m_iInstances==2);
	{ mapObjs.Add(20, InstanceCounter()); }
	assert(InstanceCounter::m_iInstances==3);
	{ mapObjs.Add(30, InstanceCounter()); }
	assert(InstanceCounter::m_iInstances==4);
	mapObjs.Remove(10);
	assert(InstanceCounter::m_iInstances==3);
	mapObjs.Clear();
	assert(InstanceCounter::m_iInstances==1);


	// Test construction/destruction when holding owned object pointers
	Dictionary<int, SharedPtr<InstanceCounter>> mapPtrs;
	mapPtrs.Add(10, new InstanceCounter());
	mapPtrs.Add(20, new InstanceCounter());
	mapPtrs.Add(30, new InstanceCounter());
	assert(InstanceCounter::m_iInstances==4);
	mapPtrs.Remove(10);
	assert(InstanceCounter::m_iInstances==3);
	mapPtrs.Clear();
	assert(InstanceCounter::m_iInstances==1);

	Dictionary<CAnsiString, int> strs;
	strs.Add("Apples", 1);
	strs.Add("Pears", 2);
	strs.Add("Bananas", 3);
	int val = 0;
	assert(!strs.TryGetValue("APPLES", val));
	assert(val == 0);

	Dictionary<CAnsiString, int, SCaseI> strsI;
	strsI.Add("Apples", 1);
	strsI.Add("Pears", 2);
	strsI.Add("Bananas", 3);
	val = 0;
	assert(strsI.TryGetValue("APPLES", val));
	assert(val == 1);

	if (!g_bFailed)
		printf("OK\n");
}

void TestKeyedArray()
{
	printf("Testing KeyedArray...");
	g_bFailed=false;

	// Start with empty ka
	KeyedArray<int, int> ka;
	assert(ka.IsEmpty());
	assert(ka.GetCount()==0);

	// Add some items
	int i;
	for (i=1; i<=100; i++)
	{
		ka.Add(i, i*10);
	}
	assert(!ka.IsEmpty());
	assert(ka.GetCount()==100);


	// Check lookups...
	assert(ka.Get(1)==10);
	assert(ka.Get(5)==50);
	assert(ka.Get(10)==100);
	assert(ka.Get(2000, -1)==-1);
	assert(ka.ContainsKey(1));
	assert(ka.ContainsKey(50));
	assert(ka.ContainsKey(100));
	assert(!ka.ContainsKey(0));
	assert(!ka.ContainsKey(101));

	// Check find  (Get above uses Find so just one test...)
	int iValue;
	assert(ka.TryGetValue(1, iValue)==true && iValue==10);

	// Test iteration
	for (i=0; i<ka.GetCount(); i++)
	{
		assert(ka.GetAt(i).Key==i+1);
		assert(ka.GetAt(i).Value==ka.GetAt(i).Key*10);
	}

	// Test iteration while removing... remove all odd elements
	// NB: Behaviour when forward iterating and removing should be same as for
	//		vector, where when removing current element, iteration index should be decremented
	int iTotal=ka.GetCount();
	int iIndex=0;
	for (i=0; i<ka.GetCount(); i++, iIndex++)
	{
		int iKey=ka.GetAt(i).Key;
		assert(iKey==iIndex+1);

		if ((iKey%2)!=0)
		{
			ka.Remove(ka.GetAt(i).Key);
			iTotal--;
			i--;
		}
	}
	assert(ka.GetCount()==iTotal);

	// Check only evens left...
	assert(ka.GetCount()==50);
	for (i=0; i<ka.GetCount(); i++)
	{
		assert((ka.GetAt(i).Key%2)==0);
	}

	// Test reverse iteration while removing
	for (i=ka.GetCount()-1; i>=0; i--)
	{
		if ((ka.GetAt(i).Key % 10)==0)
		{
			ka.Remove(ka.GetAt(i).Key);
			iTotal--;
		}
	}
	assert(ka.GetCount()==iTotal);

	// Repopulate...
	ka.Clear();
	assert(ka.GetCount()==0);
	assert(ka.IsEmpty());
	for (i=0; i<100; i++)
	{
		ka.Add(i*10, i*10);
	}

	// Test construction/destruction when holding objects
	// NB: There's an extra 1 on the reference count because of the instance in
	//		the m_Leaf member of the ka itself.
	KeyedArray<int, InstanceCounter>	kaObjs;
	{ kaObjs.Add(10, InstanceCounter()); }
	assert(InstanceCounter::m_iInstances==1);
	{ kaObjs.Add(20, InstanceCounter()); }
	assert(InstanceCounter::m_iInstances==2);
	{ kaObjs.Add(30, InstanceCounter()); }
	assert(InstanceCounter::m_iInstances==3);
	kaObjs.Remove(10);
	assert(InstanceCounter::m_iInstances==2);
	kaObjs.Clear();
	assert(InstanceCounter::m_iInstances==0);


	// Test construction/destruction when holding owned object pointers
	KeyedArray<int, SharedPtr<InstanceCounter>> kaPtrs;
	kaPtrs.Add(10, new InstanceCounter());
	kaPtrs.Add(20, new InstanceCounter());
	kaPtrs.Add(30, new InstanceCounter());
	assert(InstanceCounter::m_iInstances==3);
	kaPtrs.Remove(10);
	assert(InstanceCounter::m_iInstances==2);
	kaPtrs.Clear();
	assert(InstanceCounter::m_iInstances==0);

	KeyedArray<CAnsiString, int> strs;
	strs.Add("Apples", 1);
	strs.Add("Pears", 2);
	strs.Add("Bananas", 3);
	int val = 0;
	assert(!strs.TryGetValue("APPLES", val));
	assert(val == 0);

	KeyedArray<CAnsiString, int, SCaseI> strsI;
	strsI.Add("Apples", 1);
	strsI.Add("Pears", 2);
	strsI.Add("Bananas", 3);
	val = 0;
	assert(strsI.TryGetValue("APPLES", val));
	assert(val == 1);

	if (!g_bFailed)
		printf("OK\n");
}

void TestFormatting()
{
	printf("Testing Formatting...");
	g_bFailed=false;

	// Format
	assert(CAnsiString::Format("%s World %i", "Hello", 23).IsEqualTo("Hello World 23"));

	// Unicode Format
	CUniString strU=L"Hello";
	CUniString strR=CUniString::Format(L"%s World %i", strU.sz(), 24);
	assert(strR.IsEqualTo(L"Hello World 24"));

	assert(CUniString::Format(L"%f", 123.456).IsEqualTo(L"123.456000"));
	assert(CUniString::Format(L"%.3f", 123.456).IsEqualTo(L"123.456"));
	assert(CUniString::Format(L"%.*f", 3, 123.456).IsEqualTo(L"123.456"));
	assert(CUniString::Format(L"%+.3f", 123.456).IsEqualTo(L"+123.456"));
	assert(CUniString::Format(L"% .3f", 123.456).IsEqualTo(L" 123.456"));

	if (!g_bFailed)
		printf("OK\n");

}

void TestEncoding()
{
	printf("Testing Encoding...");
	g_bFailed=false;

	// Test utf8 -> utf32
	assert((Encode<char32_t>("\x24").IsEqualTo(U"\x24")));
	assert((Encode<char32_t>("\xC2\xA2").IsEqualTo(U"\xA2")));
	assert((Encode<char32_t>("\xE2\x82\xAC").IsEqualTo(U"\u20AC")));
	assert((Encode<char32_t>("\xED\x95\x9C").IsEqualTo(U"\uD55C")));
	assert((Encode<char32_t>("\xF0\x90\x8D\x88").IsEqualTo(U"\U00010348")));

	// Test utf8 -> utf16
	assert((Encode<char16_t>("\x24").IsEqualTo(u"\x24")));
	assert((Encode<char16_t>("\xC2\xA2").IsEqualTo(u"\xA2")));
	assert((Encode<char16_t>("\xE2\x82\xAC").IsEqualTo(u"\u20AC")));
	assert((Encode<char16_t>("\xED\x95\x9C").IsEqualTo(u"\uD55C")));
	assert((Encode<char16_t>("\xF0\x90\x8D\x88").IsEqualTo(u"\U00010348")));

	// Test utf16 -> utf8
	assert((Encode<char>(u"\x24").IsEqualTo("\x24")));
	assert((Encode<char>(u"\xA2").IsEqualTo("\xC2\xA2")));
	assert((Encode<char>(u"\u20AC").IsEqualTo("\xE2\x82\xAC")));
	assert((Encode<char>(u"\uD55C").IsEqualTo("\xED\x95\x9C")));
	assert((Encode<char>(u"\U00010348").IsEqualTo("\xF0\x90\x8D\x88")));

	// Test utf16 -> utf32
	assert((Encode<char32_t>(u"\x24").IsEqualTo(U"\x24")));
	assert((Encode<char32_t>(u"\u20AC").IsEqualTo(U"\u20AC")));
	assert((Encode<char32_t>(u"\U00010437").IsEqualTo(U"\U00010437")));
	assert((Encode<char32_t>(u"\U00024B62").IsEqualTo(U"\U00024B62")));

	// Test utf32 -> utf8
	assert((Encode<char>(U"\x24").IsEqualTo("\x24")));
	assert((Encode<char>(U"\xA2").IsEqualTo("\xC2\xA2")));
	assert((Encode<char>(U"\u20AC").IsEqualTo("\xE2\x82\xAC")));
	assert((Encode<char>(U"\uD55C").IsEqualTo("\xED\x95\x9C")));
	assert((Encode<char>(U"\U00010348").IsEqualTo("\xF0\x90\x8D\x88")));

	// Test utf32 -> utf16
	assert((Encode<char16_t>(U"\x24").IsEqualTo(u"\x24")));
	assert((Encode<char16_t>(U"\u20AC").IsEqualTo(u"\u20AC")));
	assert((Encode<char16_t>(U"\U00010437").IsEqualTo(u"\U00010437")));
	assert((Encode<char16_t>(U"\U00024B62").IsEqualTo(u"\U00024B62")));


	if (!g_bFailed)
		printf("OK\n");

}

void TestJson()
{
	printf("Testing JSON...");
	g_bFailed=false;

	assert(JSON::Stringify(SharedPtr<JsonValue>(new JsonNull()), 0).IsEqualTo("null"));
	assert(JSON::Stringify(SharedPtr<JsonValue>(new JsonBoolean(false)), 0).IsEqualTo("false"));
	assert(JSON::Stringify(SharedPtr<JsonValue>(new JsonBoolean(true)), 0).IsEqualTo("true"));
	assert(JSON::Stringify(SharedPtr<JsonValue>(new JsonString("Hello World")), 0).IsEqualTo("\"Hello World\""));
	assert(JSON::Stringify(SharedPtr<JsonValue>(new JsonString("Hello \n World")), 0).IsEqualTo("\"Hello \\n World\""));

	SharedPtr<JsonArray> arr = new JsonArray();
	arr->Add(new JsonNumber(10));
	arr->Add(new JsonNumber(20));
	arr->Add(new JsonNumber(30));
	assert(JSON::Stringify(arr, 0).IsEqualTo("[10,20,30]"));

	arr = new JsonArray();
	arr->Add(new JsonNumber(10.123));
	arr->Add(new JsonNumber(20.234));
	arr->Add(new JsonNumber(30.345));
	assert(JSON::Stringify(arr, 0).IsEqualTo("[10.123,20.234,30.345]"));

	if (!g_bFailed)
		printf("OK\n");
}


/*
void TestPath()
{
	printf("Testing Path...");
	g_bFailed=false;

#ifdef _WIN32
	assert(Path<>::Join("\\a", "\\b").IsEqualTo("\\a\\b"));
	assert(Path<>::Join("\\a", "b").IsEqualTo("\\a\\b"));
	assert(Path<>::Join("\\a\\", "b").IsEqualTo("\\a\\b"));
	assert(Path<>::Join("\\a\\", "\\b").IsEqualTo("\\a\\b"));

	assert(Path<>::GetFileName("\\a\\file.txt").IsEqualTo("file.txt"));
	assert(Path<>::GetFileName("\\a\\file.txt").IsEqualTo("file.txt"));

	assert(Path<>::GetDirectoryName("\\a\\b\\c\\file.txt").IsEqualTo("\\a\\b\\c"));
	assert(Path<>::GetDirectoryName("\\a\\b\\c\\").IsEqualTo("\\a\\b\\c"));
	assert(Path<>::GetDirectoryName("\\a\\b\\c").IsEqualTo("\\a\\b"));

	assert(Path<>::GetDirectoryName("C:\\MyDir").IsEqualTo("C:\\"));
	assert(Path<>::GetDirectoryName("C:\\").IsEmpty());
	assert(Path<>::GetDirectoryName("\\\\unc\\share\\dir").IsEqualTo("\\\\unc\\share"));
	assert(Path<>::GetDirectoryName("\\\\unc\\share").IsEmpty());

	assert(Path<>::IsFullyQualified("C:") == false);
	assert(Path<>::IsFullyQualified("C:\\") == true);
	assert(Path<>::IsFullyQualified("C:\\file") == true);
	assert(Path<>::IsFullyQualified("C:file") == false);
	assert(Path<>::IsFullyQualified("\\\\unc\\share") == true);
	assert(Path<>::IsFullyQualified("\\\\unc\\share\\") == true);
	assert(Path<>::IsFullyQualified("\\\\unc\\share\\file") == true);

	assert(Path<>::GetFileNameWithoutExtension("\\a\\file.txt").IsEqualTo("file"));
	assert(Path<>::GetFileNameWithoutExtension("\\a\\file.").IsEqualTo("file"));
	assert(Path<>::GetFileNameWithoutExtension("\\a\\file").IsEqualTo("file"));
	
	assert(Path<>::GetExtension("\\a\\file.txt").IsEqualTo(".txt"));
	assert(Path<>::GetExtension("\\a.dir\\file").IsEmpty());

	assert(Path<>::ChangeExtension("\\a\\file.txt", "doc").IsEqualTo("\\a\\file.doc"));
	assert(Path<>::ChangeExtension("\\a\\file", "doc").IsEqualTo("\\a\\file.doc"));
	assert(Path<>::ChangeExtension("\\a\\file.txt", ".doc").IsEqualTo("\\a\\file.doc"));
	assert(Path<>::ChangeExtension("\\a\\file", ".doc").IsEqualTo("\\a\\file.doc"));

	assert(Path<>::Canonicalize("\\a\\b\\c").IsEqualTo("\\a\\b\\c"));
	assert(Path<>::Canonicalize("\\a\\.\\b\\.\\c").IsEqualTo("\\a\\b\\c"));
	assert(Path<>::Canonicalize("\\a\\..\\b\\c").IsEqualTo("\\b\\c"));
	assert(Path<>::Canonicalize("\\a\\b\\..\\..\\c").IsEqualTo("\\c"));
	assert(Path<>::Canonicalize("a\\b\\c").IsEqualTo("a\\b\\c"));
	assert(Path<>::Canonicalize("C:\\a\\b\\c").IsEqualTo("C:\\a\\b\\c"));
	assert(Path<>::Canonicalize("\\\\a\\b\\c").IsEqualTo("\\\\a\\b\\c"));
	assert(Path<>::Canonicalize("\\\\a\\b\\\\c").IsEqualTo("\\\\a\\b\\c"));
	assert(Path<>::Canonicalize("\\a\\b\\c\\").IsEqualTo("\\a\\b\\c\\"));
	assert(Path<>::Canonicalize("\\a\\b\\c\\\\\\\\").IsEqualTo("\\a\\b\\c\\"));

	assert(Path<>::Combine("\\a", "b").IsEqualTo("\\a\\b"));
	assert(Path<>::Combine("\\a", "\\b").IsEqualTo("\\b"));
	assert(Path<>::Combine("C:\\a", "\\b").IsEqualTo("C:\\b"));
	assert(Path<>::Combine("\\\\unc\\share\\subdir", "\\b").IsEqualTo("\\\\unc\\share\\b"));
	assert(Path<>::Combine("\\a", "b\\subdir\\..\\otherdir\\c").IsEqualTo("\\a\\b\\otherdir\\c"));
	assert(Path<>::Combine("\\a", "b\\subdir\\..\\otherdir\\c\\").IsEqualTo("\\a\\b\\otherdir\\c\\"));

#else
	assert(CPath<>::Join("/a", "/b").IsEqualTo("/a/b"));
	assert(CPath<>::Join("/a", "b").IsEqualTo("/a/b"));
	assert(CPath<>::Join("/a/", "b").IsEqualTo("/a/b"));
	assert(CPath<>::Join("/a/", "/b").IsEqualTo("/a/b"));

	assert(CPath<>::GetFileName("/a/file.txt").IsEqualTo("file.txt"));
	
	assert(CPath<>::GetDirectoryName("/a/b/c/file.txt").IsEqualTo("/a/b/c"));
	assert(CPath<>::GetDirectoryName("/a/b/c/").IsEqualTo("/a/b/c"));
	assert(CPath<>::GetDirectoryName("/a/b/c").IsEqualTo("/a/b"));

	assert(CPath<>::GetFileNameWithoutExtension("/a/file.txt").IsEqualTo("file"));
	assert(CPath<>::GetFileNameWithoutExtension("/a/file.").IsEqualTo("file"));
	assert(CPath<>::GetFileNameWithoutExtension("/a/file").IsEqualTo("file"));
	
	assert(CPath<>::GetExtension("/a/file.txt").IsEqualTo(".txt"));
	assert(CPath<>::GetExtension("/a.dir/file").IsEmpty());

	assert(CPath<>::ChangeExtension("/a/file.txt", "doc").IsEqualTo("/a/file.doc"));
	assert(CPath<>::ChangeExtension("/a/file", "doc").IsEqualTo("/a/file.doc"));
	assert(CPath<>::ChangeExtension("/a/file.txt", ".doc").IsEqualTo("/a/file.doc"));
	assert(CPath<>::ChangeExtension("/a/file", ".doc").IsEqualTo("/a/file.doc"));

	assert(CPath<>::Canonicalize("/a/b/c").IsEqualTo("/a/b/c"));
	assert(CPath<>::Canonicalize("/a/./b/./c").IsEqualTo("/a/b/c"));
	assert(CPath<>::Canonicalize("/a/../b/c").IsEqualTo("/b/c"));
	assert(CPath<>::Canonicalize("/a/b/../../c").IsEqualTo("/c"));
	assert(CPath<>::Canonicalize("a/b/c").IsEqualTo("a/b/c"));
	assert(CPath<>::Canonicalize("/a/b/c/").IsEqualTo("/a/b/c/"));
	assert(CPath<>::Canonicalize("/a/b/c////").IsEqualTo("/a/b/c/"));

	assert(CPath<>::Combine("/a", "b").IsEqualTo("/a/b"));
	assert(CPath<>::Combine("/a", "/b").IsEqualTo("/b"));
	assert(CPath<>::Combine("/a", "b/subdir/../otherdir/c").IsEqualTo("/a/b/otherdir/c"));
	assert(CPath<>::Combine("/a", "b/subdir/../otherdir/c/").IsEqualTo("/a/b/otherdir/c/"));
#endif

	if (!g_bFailed)
		printf("OK\n");

}

void TestStream(Stream& s)
{
	assert(s.IsOpen());
	assert(s.GetLength() == 0);

	// Write integers
	int count = 10;
	for (int i=0; i<10; i++)
	{
		s.Write(i);
	}

	// Check
	assert(s.GetLength() == sizeof(int) * 10);
	assert(s.GetLength() == s.Tell());

	// Read back in reverse order
	for (int i=count-1; i>=0; i--)
	{
		s.Seek(i * sizeof(int));
		assert((size_t)s.Tell() == i * sizeof(int));
		int val;
		s.Read(val);
		assert(val == i);
		assert((size_t)s.Tell() == (i+1) * sizeof(int));
	}

	// EOF test
	s.Seek(s.GetLength());
	assert(!s.IsEof());
	int temp;
	s.Read(temp);
	assert(s.IsEof());

	// Read all
	int x[100];
	s.Seek(0);
	size_t cbRead;
	s.Read(x, sizeof(x), &cbRead);
	assert(cbRead = sizeof(int) * count);

	// Truncate
	s.Seek(sizeof(int) * count / 2);
	s.Truncate();
	assert((size_t)s.GetLength() == sizeof(int) * count / 2);

	// Close
	s.Close();
	assert(!s.IsOpen());
}

void TestFileStream()
{
	printf("Testing File Stream...");
	g_bFailed=false;

	FileStream s;
	s.Create("test.bin");
	TestStream(s);

	if (!g_bFailed)
		printf("OK\n");

}

void TestMemoryStream()
{
	printf("Testing Memory Stream...");
	g_bFailed=false;

	MemoryStream s;
	s.Create();
	TestStream(s);

	if (!g_bFailed)
		printf("OK\n");
}

void TestFile()
{
	printf("Testing File...");
	g_bFailed=false;

	// Read this file
	CString str;
	File::ReadAllText("SimpleTest.cpp", str);
	assert(str.StartsWith("// SimpleTest.cpp"));

	// Write it again
	File::WriteAllText("test.bin", str.sz());

	// Read it again
	CString str2;
	File::ReadAllText("test.bin", str2);
	assert(str.IsEqualTo(str2));

	// Copy/delete/exists
	assert(File::Exists("test.bin"));
	assert(File::Copy("test.bin", "copy.bin", true) == 0);
	assert(File::Exists("copy.bin"));
	assert(File::Copy("test.bin", "copy.bin", true) == 0);
	assert(File::Exists("copy.bin"));
	assert(File::Copy("test.bin", "copy.bin", false) == EEXIST);
	File::Delete("test.bin");
	assert(!File::Exists("test.bin"));
	File::Delete("copy.bin");
	assert(!File::Exists("copy.bin"));

	if (!g_bFailed)
		printf("OK\n");
}


void TestDirectory()
{
	printf("Testing Directory...");
	g_bFailed=false;

	assert((Path<>::DoesMatchPattern<char>("file.txt", "file.txt")));
	assert((Path<>::DoesMatchPattern<char>("file.txt", "file.*")));
	assert((Path<>::DoesMatchPattern<char>("file.txt", "*.txt")));
	assert((Path<>::DoesMatchPattern<char>("file.txt", "fi*.txt")));
	assert((Path<>::DoesMatchPattern<char>("file.txt", "*.*")));
	assert((Path<>::DoesMatchPattern<char>("file.txt", "*")));
	assert((Path<>::DoesMatchPattern<char>("file.txt", "fi??.*")));

#ifdef _WIN32
	assert((Path<>::DoesMatchPattern<char>("file.txt", "FILE.*")));
#else
	assert(!(CPath<>::DoesMatchPattern<char>("file.txt", "FILE.*")));
#endif

	assert(!Directory<>::Exists("temp"));
	assert(!Directory<>::Create("temp"));
	assert(Directory<>::Exists("temp"));
	assert(!Directory<>::Delete("temp"));
	assert(!Directory<>::Exists("temp"));

	DirectoryIterator iter;
	Directory<>::Iterate("..", "*.h", IterateFlags::All, iter);
	while (iter.Next())
	{
		printf("%s\n", iter.Name);
	}

	if (!g_bFailed)
		printf("OK\n");
}
*/

// Main entry point
int main(int argc, char* argv[])
{
	printf("SimpleLib Unit Test Cases\n");

	TestMath();
	TestVector();
	TestRectangle();
	TestStrings();
	TestList();
	TestMap();
	TestKeyedArray();
	TestFormatting();
	TestEncoding();
	TestJson();
	/*
	TestPath();
	TestFileStream();
	TestMemoryStream();
	TestFile();
	TestDirectory();
	*/

	if (g_bAnyFailed)
	{
		printf("Finished - errors found.\n\n");
	}
	else
	{
		printf("Finished - all tests passed.\n\n");
	}

	return 0;
}

