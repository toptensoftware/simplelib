// SimpleTest.cpp : SimpleLib Unit Tests


#define SIMPLELIB_POSIX_PATHS
#include "../SimpleLib.h"

using namespace SimpleLib;

bool g_bAnyFailed=false;
bool g_bFailed=false;

typedef CCoreString<char> CAnsiString;
typedef CCoreString<wchar_t> CUniString;

void Failed(int iLine, const char* psz)
{
	if (!g_bFailed) printf("\n");
	g_bAnyFailed=true;
	g_bFailed=true;
	printf("Failed(%i): %s\n", iLine, psz);
}

#undef assert
#define assert(x)  if (!(x)) Failed(__LINE__, #x);

class CInstanceCounter
{
public:
	CInstanceCounter()
	{
		m_iInstances++;
	}

	CInstanceCounter(const CInstanceCounter& Other)
	{
		m_iInstances++;
	}

	virtual ~CInstanceCounter()
	{
		m_iInstances--;
	}

	static int m_iInstances;
};

int CInstanceCounter::m_iInstances=0;

class CMyObject
{
public:
	CMyObject(int iVal) : m_iVal(iVal)
	{
	}
	CMyObject(const CMyObject& Other) : m_iVal(Other.m_iVal)
	{
	}

	int m_iVal;
};

/*
namespace SimpleLib
{
	template <>
	int Compare(CMyObject* const& a, CMyObject* const& b)
	{
		return a->m_iVal-b->m_iVal;
	}

	template <>
	int Compare(const CMyObject& a, const CMyObject& b)
	{
		return a.m_iVal-b.m_iVal;
	}
}
*/


void TestStrings()
{
	g_bFailed=false;
	printf("Testing CCoreString...");


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

	CVector<CAnsiString> parts;
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

void TestVector()
{
	g_bFailed=false;
	printf("Testing CVector...");

	// Setup a vector
	CVector<int> vec;

	vec.Add(10);
	assert(vec.IndexOf(10) == 0);
	vec.Clear();

	// Add 10 items
	int i;
	for (i=0; i<10; i++)
	{
		vec.Add(i);
	}

	// Test initial addd
	assert(vec.GetCount()==10);
	for (i=0; i<10; i++)
	{
		assert(vec[i]==i);
	}

	// Insert At
	vec.InsertAt(5, 100);
	assert(vec[4]==4);
	assert(vec[5]==100);
	assert(vec[6]==5);

	// Remove At
	vec.RemoveAt(5);
	assert(vec[4]==4);
	assert(vec[5]==5);

	// Remove At (multiple)
	vec.InsertAt(5, 100);
	vec.InsertAt(5, 100);
	vec.InsertAt(5, 100);
	vec.InsertAt(5, 100);
	vec.RemoveAt(5, 4);
	assert(vec[4]==4);
	assert(vec[5]==5);

	// Swap
	vec.Swap(1,2);
	assert(vec[0]==0);
	assert(vec[1]==2);
	assert(vec[2]==1);
	assert(vec[3]==3);
	vec.Swap(1,2);

	// Move
	vec.Move(3,1);
	assert(vec[0]==0);
	assert(vec[1]==3);
	assert(vec[2]==1);
	assert(vec[3]==2);

	// Move back
	vec.Move(1,3);
	assert(vec[0]==0);
	assert(vec[1]==1);
	assert(vec[2]==2);
	assert(vec[3]==3);

	// Find
	assert(vec.IndexOf(100)==-1);
	assert(vec.IndexOf(8)==8);

	// Find with start position
	vec.Add(1);
	assert(vec.IndexOf(1)==1);
	assert(vec.IndexOf(1,5)==10);
	assert(!vec.IsEmpty());

	// Remove All
	vec.Clear();
	assert(vec.GetCount()==0);
	assert(vec.IsEmpty());

	// Test stack operations
	vec.Clear();
	vec.Push(10);
	vec.Push(20);
	vec.Push(30);
	assert(vec[0]==10);
	assert(vec[1]==20);
	assert(vec[2]==30);
	assert(vec.Tail()==30);
	assert(vec.Pop()==30);
	assert(vec.Pop()==20);
	assert(vec.Pop()==10);
	assert(vec.IsEmpty());

	// Test queue operations
	vec.Clear();
	vec.Enqueue(10);
	vec.Enqueue(20);
	vec.Enqueue(30);
	assert(vec[0]==10);
	assert(vec[1]==20);
	assert(vec[2]==30);
	assert(vec.Head()==10);
	assert(vec.Dequeue()==10);
	assert(vec.Dequeue()==20);
	assert(vec.Dequeue()==30);
	assert(vec.IsEmpty());

	// Test construction/destruction when holding objects
	CVector<CInstanceCounter>	vecObjs;
	{
	vecObjs.Add(CInstanceCounter());
	vecObjs.Add(CInstanceCounter());
	vecObjs.Add(CInstanceCounter());
	}
	assert(CInstanceCounter::m_iInstances==3);
	vecObjs.RemoveAt(0);
	assert(CInstanceCounter::m_iInstances==2);
	vecObjs.Clear();
	assert(CInstanceCounter::m_iInstances==0);


	// Test construction/destruction when holding owned object pointers
	CVector<CInstanceCounter*, SOwnedPtr> vecPtrs;
	vecPtrs.Add(new CInstanceCounter());
	vecPtrs.Add(new CInstanceCounter());
	vecPtrs.Add(new CInstanceCounter());
	assert(CInstanceCounter::m_iInstances==3);
	vecPtrs.RemoveAt(0);
	assert(CInstanceCounter::m_iInstances==2);
	CInstanceCounter* pDetach=vecPtrs.DetachAt(0);
	assert(CInstanceCounter::m_iInstances==2);
	vecPtrs.Clear();
	assert(CInstanceCounter::m_iInstances==1);
	delete pDetach;
	assert(CInstanceCounter::m_iInstances==0);

	CVector<CUniString, SString<wchar_t>> strs;
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
	printf("Testing CMap...");
	g_bFailed=false;

	// Start with empty map
	CMap<int, int> map;
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
	map.RemoveAll();
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
	CMap<int, CInstanceCounter>	mapObjs;
	{ mapObjs.Add(10, CInstanceCounter()); }
	assert(CInstanceCounter::m_iInstances==2);
	{ mapObjs.Add(20, CInstanceCounter()); }
	assert(CInstanceCounter::m_iInstances==3);
	{ mapObjs.Add(30, CInstanceCounter()); }
	assert(CInstanceCounter::m_iInstances==4);
	mapObjs.Remove(10);
	assert(CInstanceCounter::m_iInstances==3);
	mapObjs.RemoveAll();
	assert(CInstanceCounter::m_iInstances==1);


	// Test construction/destruction when holding owned object pointers
	CMap<int, CInstanceCounter*, SValue, SOwnedPtr> mapPtrs;
	mapPtrs.Add(10, new CInstanceCounter());
	mapPtrs.Add(20, new CInstanceCounter());
	mapPtrs.Add(30, new CInstanceCounter());
	assert(CInstanceCounter::m_iInstances==4);
	mapPtrs.Remove(10);
	assert(CInstanceCounter::m_iInstances==3);
	CInstanceCounter* pDetach=mapPtrs.Detach(20);
	assert(CInstanceCounter::m_iInstances==3);
	mapPtrs.RemoveAll();
	assert(CInstanceCounter::m_iInstances==2);
	delete pDetach;
	assert(CInstanceCounter::m_iInstances==1);

	CMap<CAnsiString, int> strs;
	strs.Add("Apples", 1);
	strs.Add("Pears", 2);
	strs.Add("Bananas", 3);
	int val = 0;
	assert(!strs.TryGetValue("APPLES", val));
	assert(val == 0);

	CMap<CAnsiString, int, SCaseI> strsI;
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
	printf("Testing CKeyedArray...");
	g_bFailed=false;

	// Start with empty ka
	CKeyedArray<int, int> ka;
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
	CKeyedArray<int, CInstanceCounter>	kaObjs;
	{ kaObjs.Add(10, CInstanceCounter()); }
	assert(CInstanceCounter::m_iInstances==1);
	{ kaObjs.Add(20, CInstanceCounter()); }
	assert(CInstanceCounter::m_iInstances==2);
	{ kaObjs.Add(30, CInstanceCounter()); }
	assert(CInstanceCounter::m_iInstances==3);
	kaObjs.Remove(10);
	assert(CInstanceCounter::m_iInstances==2);
	kaObjs.Clear();
	assert(CInstanceCounter::m_iInstances==0);


	// Test construction/destruction when holding owned object pointers
	CKeyedArray<int, CInstanceCounter*, SValue, SOwnedPtr> kaPtrs;
	kaPtrs.Add(10, new CInstanceCounter());
	kaPtrs.Add(20, new CInstanceCounter());
	kaPtrs.Add(30, new CInstanceCounter());
	assert(CInstanceCounter::m_iInstances==3);
	kaPtrs.Remove(10);
	assert(CInstanceCounter::m_iInstances==2);
	CInstanceCounter* pDetach=kaPtrs.Detach(20);
	assert(CInstanceCounter::m_iInstances==2);
	kaPtrs.Clear();
	assert(CInstanceCounter::m_iInstances==1);
	delete pDetach;
	assert(CInstanceCounter::m_iInstances==0);

	CKeyedArray<CAnsiString, int> strs;
	strs.Add("Apples", 1);
	strs.Add("Pears", 2);
	strs.Add("Bananas", 3);
	int val = 0;
	assert(!strs.TryGetValue("APPLES", val));
	assert(val == 0);

	CKeyedArray<CAnsiString, int, SCaseI> strsI;
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

void TestPath()
{
	printf("Testing Path...");
	g_bFailed=false;

#ifdef _WIN32
	assert(CPath::Join("\\a", "\\b").IsEqualTo("\\a\\b"));
	assert(CPath::Join("\\a", "b").IsEqualTo("\\a\\b"));
	assert(CPath::Join("\\a\\", "b").IsEqualTo("\\a\\b"));
	assert(CPath::Join("\\a\\", "\\b").IsEqualTo("\\a\\b"));

	assert(CPath::GetFileName("\\a\\file.txt").IsEqualTo("file.txt"));
	assert(CPath::GetFileName("\\a\\file.txt").IsEqualTo("file.txt"));

	assert(CPath::GetDirectoryName("\\a\\b\\c\\file.txt").IsEqualTo("\\a\\b\\c"));
	assert(CPath::GetDirectoryName("\\a\\b\\c\\").IsEqualTo("\\a\\b\\c"));
	assert(CPath::GetDirectoryName("\\a\\b\\c").IsEqualTo("\\a\\b"));

	assert(CPath::GetDirectoryName("C:\\MyDir").IsEqualTo("C:\\"));
	assert(CPath::GetDirectoryName("C:\\").IsEmpty());
	assert(CPath::GetDirectoryName("\\\\unc\\share\\dir").IsEqualTo("\\\\unc\\share"));
	assert(CPath::GetDirectoryName("\\\\unc\\share").IsEmpty());

	assert(CPath::IsFullyQualified("C:") == false);
	assert(CPath::IsFullyQualified("C:\\") == true);
	assert(CPath::IsFullyQualified("C:\\file") == true);
	assert(CPath::IsFullyQualified("C:file") == false);
	assert(CPath::IsFullyQualified("\\\\unc\\share") == true);
	assert(CPath::IsFullyQualified("\\\\unc\\share\\") == true);
	assert(CPath::IsFullyQualified("\\\\unc\\share\\file") == true);

	assert(CPath::GetFileNameWithoutExtension("\\a\\file.txt").IsEqualTo("file"));
	assert(CPath::GetFileNameWithoutExtension("\\a\\file.").IsEqualTo("file"));
	assert(CPath::GetFileNameWithoutExtension("\\a\\file").IsEqualTo("file"));
	
	assert(CPath::GetExtension("\\a\\file.txt").IsEqualTo(".txt"));
	assert(CPath::GetExtension("\\a.dir\\file").IsEmpty());

	assert(CPath::ChangeExtension("\\a\\file.txt", "doc").IsEqualTo("\\a\\file.doc"));
	assert(CPath::ChangeExtension("\\a\\file", "doc").IsEqualTo("\\a\\file.doc"));
	assert(CPath::ChangeExtension("\\a\\file.txt", ".doc").IsEqualTo("\\a\\file.doc"));
	assert(CPath::ChangeExtension("\\a\\file", ".doc").IsEqualTo("\\a\\file.doc"));

	assert(CPath::Canonicalize("\\a\\b\\c").IsEqualTo("\\a\\b\\c"));
	assert(CPath::Canonicalize("\\a\\.\\b\\.\\c").IsEqualTo("\\a\\b\\c"));
	assert(CPath::Canonicalize("\\a\\..\\b\\c").IsEqualTo("\\b\\c"));
	assert(CPath::Canonicalize("\\a\\b\\..\\..\\c").IsEqualTo("\\c"));
	assert(CPath::Canonicalize("a\\b\\c").IsEqualTo("a\\b\\c"));
	assert(CPath::Canonicalize("C:\\a\\b\\c").IsEqualTo("C:\\a\\b\\c"));
	assert(CPath::Canonicalize("\\\\a\\b\\c").IsEqualTo("\\\\a\\b\\c"));
	assert(CPath::Canonicalize("\\\\a\\b\\\\c").IsEqualTo("\\\\a\\b\\c"));
	assert(CPath::Canonicalize("\\a\\b\\c\\").IsEqualTo("\\a\\b\\c\\"));
	assert(CPath::Canonicalize("\\a\\b\\c\\\\\\\\").IsEqualTo("\\a\\b\\c\\"));

	assert(CPath::Combine("\\a", "b").IsEqualTo("\\a\\b"));
	assert(CPath::Combine("\\a", "\\b").IsEqualTo("\\b"));
	assert(CPath::Combine("C:\\a", "\\b").IsEqualTo("C:\\b"));
	assert(CPath::Combine("\\\\unc\\share\\subdir", "\\b").IsEqualTo("\\\\unc\\share\\b"));
	assert(CPath::Combine("\\a", "b\\subdir\\..\\otherdir\\c").IsEqualTo("\\a\\b\\otherdir\\c"));
	assert(CPath::Combine("\\a", "b\\subdir\\..\\otherdir\\c\\").IsEqualTo("\\a\\b\\otherdir\\c\\"));

#else
	assert(CPath::Join("/a", "/b").IsEqualTo("/a/b"));
	assert(CPath::Join("/a", "b").IsEqualTo("/a/b"));
	assert(CPath::Join("/a/", "b").IsEqualTo("/a/b"));
	assert(CPath::Join("/a/", "/b").IsEqualTo("/a/b"));

	assert(CPath::GetFileName("/a/file.txt").IsEqualTo("file.txt"));
	
	assert(CPath::GetDirectoryName("/a/b/c/file.txt").IsEqualTo("/a/b/c"));
	assert(CPath::GetDirectoryName("/a/b/c/").IsEqualTo("/a/b/c"));
	assert(CPath::GetDirectoryName("/a/b/c").IsEqualTo("/a/b"));

	assert(CPath::GetFileNameWithoutExtension("/a/file.txt").IsEqualTo("file"));
	assert(CPath::GetFileNameWithoutExtension("/a/file.").IsEqualTo("file"));
	assert(CPath::GetFileNameWithoutExtension("/a/file").IsEqualTo("file"));
	
	assert(CPath::GetExtension("/a/file.txt").IsEqualTo(".txt"));
	assert(CPath::GetExtension("/a.dir/file").IsEmpty());

	assert(CPath::ChangeExtension("/a/file.txt", "doc").IsEqualTo("/a/file.doc"));
	assert(CPath::ChangeExtension("/a/file", "doc").IsEqualTo("/a/file.doc"));
	assert(CPath::ChangeExtension("/a/file.txt", ".doc").IsEqualTo("/a/file.doc"));
	assert(CPath::ChangeExtension("/a/file", ".doc").IsEqualTo("/a/file.doc"));

	assert(CPath::Canonicalize("/a/b/c").IsEqualTo("/a/b/c"));
	assert(CPath::Canonicalize("/a/./b/./c").IsEqualTo("/a/b/c"));
	assert(CPath::Canonicalize("/a/../b/c").IsEqualTo("/b/c"));
	assert(CPath::Canonicalize("/a/b/../../c").IsEqualTo("/c"));
	assert(CPath::Canonicalize("a/b/c").IsEqualTo("a/b/c"));
	assert(CPath::Canonicalize("/a/b/c/").IsEqualTo("/a/b/c/"));
	assert(CPath::Canonicalize("/a/b/c////").IsEqualTo("/a/b/c/"));

	assert(CPath::Combine("/a", "b").IsEqualTo("/a/b"));
	assert(CPath::Combine("/a", "/b").IsEqualTo("/b"));
	assert(CPath::Combine("/a", "b/subdir/../otherdir/c").IsEqualTo("/a/b/otherdir/c"));
	assert(CPath::Combine("/a", "b/subdir/../otherdir/c/").IsEqualTo("/a/b/otherdir/c/"));
#endif

	if (!g_bFailed)
		printf("OK\n");

}

void TestStream(CStream& s)
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

	CFileStream s;
	s.Create("test.bin");
	TestStream(s);

	if (!g_bFailed)
		printf("OK\n");

}

void TestMemoryStream()
{
	printf("Testing Memory Stream...");
	g_bFailed=false;

	CMemoryStream s;
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
	CFile::ReadAllText("SimpleTest.cpp", str);
	assert(str.StartsWith("// SimpleTest.cpp"));

	// Write it again
	CFile::WriteAllText("test.bin", str.sz());

	// Read it again
	CString str2;
	CFile::ReadAllText("test.bin", str2);
	assert(str.IsEqualTo(str2));

	// Copy/delete/exists
	assert(CFile::Exists("test.bin"));
	assert(CFile::Copy("test.bin", "copy.bin", true) == 0);
	assert(CFile::Exists("copy.bin"));
	assert(CFile::Copy("test.bin", "copy.bin", true) == 0);
	assert(CFile::Exists("copy.bin"));
	assert(CFile::Copy("test.bin", "copy.bin", false) == EEXIST);
	CFile::Delete("test.bin");
	assert(!CFile::Exists("test.bin"));
	CFile::Delete("copy.bin");
	assert(!CFile::Exists("copy.bin"));

	if (!g_bFailed)
		printf("OK\n");
}


void TestDirectory()
{
	printf("Testing Directory...");
	g_bFailed=false;

	assert((CPath::DoesMatchPattern<char>("file.txt", "file.txt")));
	assert((CPath::DoesMatchPattern<char>("file.txt", "file.*")));
	assert((CPath::DoesMatchPattern<char>("file.txt", "*.txt")));
	assert((CPath::DoesMatchPattern<char>("file.txt", "fi*.txt")));
	assert((CPath::DoesMatchPattern<char>("file.txt", "*.*")));
	assert((CPath::DoesMatchPattern<char>("file.txt", "*")));
	assert((CPath::DoesMatchPattern<char>("file.txt", "fi??.*")));

#ifdef _WIN32
	assert((CPath::DoesMatchPattern<char>("file.txt", "FILE.*")));
#else
	assert(!(CPath::DoesMatchPattern<char>("file.txt", "FILE.*")));
#endif

	assert(!CDirectory::Exists("temp"));
	assert(!CDirectory::Create("temp"));
	assert(CDirectory::Exists("temp"));
	assert(!CDirectory::Delete("temp"));
	assert(!CDirectory::Exists("temp"));

	CDirectoryIterator iter;
	CDirectory::Iterate("..", "*.exe", IterateFlags::All, iter);
	while (iter.Next())
	{
		printf("%s\n", iter.Name);
	}

	if (!g_bFailed)
		printf("OK\n");
}

// Main entry point
int main(int argc, char* argv[])
{
	printf("SimpleLib Unit Test Cases\n");

	TestStrings();
	TestVector();
	TestMap();
	TestKeyedArray();
	TestFormatting();
	TestEncoding();
	TestPath();
	TestFileStream();
	TestMemoryStream();
	TestFile();
	TestDirectory();

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

