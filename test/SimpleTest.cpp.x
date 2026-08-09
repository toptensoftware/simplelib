// SimpleTest.cpp : SimpleLib Unit Tests


#define SIMPLELIB_POSIX_PATHS
#include "../Geo.h"
#include "../Core.h"
#include "../Json.h"
#include "../Threading.h"

using namespace SimpleLib;

bool g_bAnyFailed=false;
bool g_bFailed=false;

typedef String CAnsiString;
typedef WString CUniString;

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

	InstanceCounter& operator=(const InstanceCounter& Other)
	{
		m_iInstances = Other.m_iInstances;
		return *this;
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


	// Test construction/destruction when holding shared object pointers
	List<SharedPtr<InstanceCounter>> vecSharedPtrs;
	vecSharedPtrs.Add(new InstanceCounter());
	vecSharedPtrs.Add(new InstanceCounter());
	vecSharedPtrs.Add(new InstanceCounter());
	assert(InstanceCounter::m_iInstances==3);
	vecSharedPtrs.RemoveAt(0);
	assert(InstanceCounter::m_iInstances==2);
	vecSharedPtrs.Clear();
	assert(InstanceCounter::m_iInstances==0);


	// Test construction/destruction when holding owned object pointers
	List<OwnedPtr<InstanceCounter>> vecOwnedPtrs;
	vecOwnedPtrs.Add(new InstanceCounter());
	vecOwnedPtrs.Add(new InstanceCounter());
	vecOwnedPtrs.Add(new InstanceCounter());
	vecOwnedPtrs.Add(new InstanceCounter());
	assert(InstanceCounter::m_iInstances==4);
	vecOwnedPtrs.RemoveAt(0);
	assert(InstanceCounter::m_iInstances==3);

	// Detach
	{
		OwnedPtr<InstanceCounter> op = vecOwnedPtrs.DetachAt(0);
		assert(InstanceCounter::m_iInstances==3);
		InstanceCounter* p = op.Detach();
		assert(InstanceCounter::m_iInstances==3);
		delete p;
		assert(InstanceCounter::m_iInstances==2);
	}
	assert(InstanceCounter::m_iInstances==2);

	vecOwnedPtrs.Clear();
	assert(InstanceCounter::m_iInstances==0);


	List<WString> strs;
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
	printf("Testing Map...");
	g_bFailed=false;

	// Start with empty map
	Map<int, int> map;
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
	int expected = 1;
	for (auto kv = map.Iterate(); kv.Next(); )
	{
		assert(kv.GetKey() == expected);
		assert(kv.GetValue() == expected * 10);
		expected++;
	}

	// Test construction/destruction when holding objects
	Map<int, InstanceCounter>	mapObjs;
	{ mapObjs.Add(10, InstanceCounter()); }
	assert(InstanceCounter::m_iInstances==1);
	{ mapObjs.Add(20, InstanceCounter()); }
	assert(InstanceCounter::m_iInstances==2);
	{ mapObjs.Add(30, InstanceCounter()); }
	assert(InstanceCounter::m_iInstances==3);
	mapObjs.Remove(10);
	assert(InstanceCounter::m_iInstances==2);
	mapObjs.Clear();
	assert(InstanceCounter::m_iInstances==0);


	// Test construction/destruction when holding owned object pointers
	Map<int, SharedPtr<InstanceCounter>> mapSharedPtrs;
	mapSharedPtrs.Add(10, new InstanceCounter());
	mapSharedPtrs.Add(20, new InstanceCounter());
	mapSharedPtrs.Add(30, new InstanceCounter());
	assert(InstanceCounter::m_iInstances==3);
	mapSharedPtrs.Remove(10);
	assert(InstanceCounter::m_iInstances==2);
	mapSharedPtrs.Clear();
	assert(InstanceCounter::m_iInstances==0);


	Map<int, OwnedPtr<InstanceCounter>> mapOwnedPtrs;
	mapOwnedPtrs.Add(10, new InstanceCounter());
	mapOwnedPtrs.Add(20, new InstanceCounter());
	mapOwnedPtrs.Add(30, new InstanceCounter());
	assert(InstanceCounter::m_iInstances==3);
	mapOwnedPtrs.Remove(10);
	assert(InstanceCounter::m_iInstances==2);
	mapOwnedPtrs.Clear();
	assert(InstanceCounter::m_iInstances==0);


	Map<CAnsiString, int> strs;
	strs.Add("Apples", 1);
	strs.Add("Pears", 2);
	strs.Add("Bananas", 3);
	int val = 0;
	assert(!strs.TryGetValue("APPLES", val));
	assert(val == 0);

/*
	for (auto iter = strs.Iterate(); iter.Next(); )
	{
		printf("%s %i\n", iter.GetKey().sz(), iter.GetValue());
	}
*/

/*
	Case insensitive hash map not supported
	Map<CAnsiString, int, SCaseI> strsI;
	strsI.Add("Apples", 1);
	strsI.Add("Pears", 2);
	strsI.Add("Bananas", 3);
	val = 0;
	assert(strsI.TryGetValue("APPLES", val));
	assert(val == 1);
*/


	if (!g_bFailed)
		printf("OK\n");
}

void TestSet()
{
	printf("Testing Set...");
	g_bFailed=false;

	// Start with empty map
	Set<int> set;
	assert(set.IsEmpty());
	assert(set.GetCount()==0);

	// Add some items
	int i;
	for (i=1; i<=100; i++)
	{
		set.Add(i);
	}
	assert(!set.IsEmpty());
	assert(set.GetCount()==100);


	// Check lookups...
	assert(set.Contains(1));
	assert(set.Contains(50));
	assert(set.Contains(100));
	assert(!set.Contains(0));
	assert(!set.Contains(101));

	// Test iteration
	int expected = 1;
	for (auto kv = set.Iterate(); kv.Next(); )
	{
		assert(kv.Get() == expected);
		expected++;
	}


	Set<CAnsiString> strs;
	strs.Add("Apples");
	strs.Add("Pears");
	strs.Add("Bananas");
	assert(strs.Contains("Apples"));
	assert(!strs.Contains("APPLES"));

/*
	for (auto iter = strs.Iterate(); iter.Next(); )
	{
		printf("%s %i\n", iter.GetKey().sz(), iter.GetValue());
	}
*/

/*
	Case insensitive hash map not supported
	Map<CAnsiString, int, SCaseI> strsI;
	strsI.Add("Apples", 1);
	strsI.Add("Pears", 2);
	strsI.Add("Bananas", 3);
	val = 0;
	assert(strsI.TryGetValue("APPLES", val));
	assert(val == 1);
*/
	Set<void*> ptrset;
	ptrset.Add(nullptr);


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

	JSONValue v;
	v = 22;
	assert(v.AsNumber() == 22.0);
	v = "Hello World";
	assert(strcmp(v.AsString(), "Hello World") == 0);
	v = true;
	assert(v.AsBool() == true);
	v.Reset();
	assert(v.kind == JsonKind::Null);

	v = new JSONArray();
	v.AsArray().Add(20);
	v.AsArray().Add(30);
	v.AsArray().Add(40);
	assert(v.kind == JsonKind::Array);
	assert(v.AsArray().GetCount() == 3);

	JSONValue v2 = v;
	assert(v.kind == JsonKind::Array);
	assert(v.AsArray().GetCount() == 3);

	v = new JSONObject();
	v.AsObject().Add("apples", "red");
	v.AsObject().Add("pears", "green");
	v.AsObject().Add("bananas", "yellow");
	assert(v.AsObject().GetCount() == 3);



	assert(JSON::Stringify(JSONValue(), 0).IsEqualTo("null"));
	assert(JSON::Stringify(false, 0).IsEqualTo("false"));
	assert(JSON::Stringify(true, 0).IsEqualTo("true"));
	assert(JSON::Stringify("Hello World", 0).IsEqualTo("\"Hello World\""));
	assert(JSON::Stringify("Hello \n World", 0).IsEqualTo("\"Hello \\n World\""));

	auto arr = new JSONArray();
	arr->Add(10);
	arr->Add(20);
	arr->Add(30);
	assert(JSON::Stringify(arr, 0).IsEqualTo("[10,20,30]"));

	arr = new JSONArray();
	arr->Add(10E123);
	arr->Add(20.234);
	arr->Add(30.345);
	arr->Add(0.00045);
	//assert(JSON::Stringify(arr, 0).IsEqualTo("[1E124,20.234,30.345,0.00045]"));

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

struct X
{
	int x;
	int y;
	int z;
};

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
	TestSet();
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

	Atomic<uint32_t> test;
	test.Inc();
	test.Dec();
	test.Add(10);
	test.FetchAdd(10);
	test.Set(1);
	test.Get();
	test.Wait(32);
	test.WakeOne();
	test.WakeAll();

	LockFreeRingBuffer<int> rb(10);
	rb.MustWrite(10);
	int x;
	rb.Read(x);

	MpmcQueue<int> q(10);
	q.MustWrite(10);
	q.Read(x);

	Thread* t;
	t->SetDescription("Hello World");
	t->SetPriority(ThreadPriority::AboveNormal);

	Mutex mx;
	EnterMutex emx(mx);

	SlimLock sl;
	EnterSlimLock esl(sl, true);

	Semaphore sema(10);
	sema.Release();

	X xx;
	AtomicStruct<X> ax(xx);

	ThreadLocal<X> tls;
	X* xptr = tls.Get();

	CowList<int> cowList;
	auto& snap = cowList.GetSnapshot();

	Crc32::Calculate("blah", 4);;

	Guid guid;
	Guid::Parse("asd", guid);

	return 0;
}

