#include "../UnitTesting.h"
#include "../Core.h"
using namespace SimpleLib;

// Tracks live instances so we can verify the list correctly constructs and
// destroys elements as they're added, removed and cleared.
class InstanceCounter
{
public:
	InstanceCounter(int iValue = 0) : Value(iValue) { s_iInstances++; }
	InstanceCounter(const InstanceCounter& other) : Value(other.Value) { s_iInstances++; }
	~InstanceCounter() { s_iInstances--; }

	int Value;

	inline static int s_iInstances = 0;
};

static int CompareInts(const int& a, const int& b)
{
	return a > b ? 1 : a < b ? -1 : 0;
}

static int CompareIntsWithContext(const int& a, const int& b, void* pUser)
{
	int iSign = *(int*)pUser;
	return (a > b ? 1 : a < b ? -1 : 0) * iSign;
}

Fact("List Basics")
{
	List<int> list;
	list.Add(10);
	list.Add(20);
	list.Add(30);
	Assert(list.GetCount() == 3);
	Assert(list[0] == 10);
	Assert(list[1] == 20);
	Assert(list[2] == 30);
}

Fact("List Empty By Default")
{
	List<int> list;
	Assert(list.GetCount() == 0);
	Assert(list.IsEmpty());
	Assert(list.GetBuffer() == nullptr);
}

Fact("List Add And IndexOf")
{
	List<int> list;
	list.Add(10);
	list.Add(20);
	list.Add(30);
	list.Add(20);
	Assert(list.IndexOf(20) == 1);
	Assert(list.IndexOf(20, 1) == 3);	// Start search after index 1
	Assert(list.IndexOf(999) == -1);
	Assert(list.Contains(30));
	Assert(!list.Contains(999));
}

Fact("List Iterate")
{
	List<int> list;
	list.Add(10);
	list.Add(20);
	list.Add(30);

	int iSeen = 0;
	int iSum = 0;
	auto it = list.Iterate();
	while (it.Next())
	{
		iSeen++;
		iSum += it.Get();
	}
	Assert(iSeen == 3);
	Assert(iSum == 60);
}

Fact("List Iterate Visits In Order")
{
	List<int> list;
	for (int i = 0; i < 10; i++)
		list.Add(i);

	int expected = 0;
	auto it = list.Iterate();
	while (it.Next())
	{
		Assert(it.Get() == expected);
		expected++;
	}
	Assert(expected == 10);
}

Fact("List Iterate Empty List")
{
	List<int> list;
	auto it = list.Iterate();
	Assert(!it.Next());
}

Fact("List SetCapacity Preserves Contents")
{
	List<int> list;
	for (int i = 0; i < 100; i++)
		list.Add(i);
	Assert(list.GetCount() == 100);
	for (int i = 0; i < 100; i++)
		Assert(list[i] == i);

	// Growing capacity ahead of time must not disturb existing contents
	list.SetCapacity(500);
	Assert(list.GetCount() == 100);
	for (int i = 0; i < 100; i++)
		Assert(list[i] == i);
}

Fact("List InsertAt")
{
	List<int> list;
	for (int i = 0; i < 10; i++)
		list.Add(i);

	list.InsertAt(5, 100);
	Assert(list.GetCount() == 11);
	Assert(list[4] == 4);
	Assert(list[5] == 100);
	Assert(list[6] == 5);

	// Insert at the head
	list.InsertAt(0, -1);
	Assert(list[0] == -1);
	Assert(list.GetCount() == 12);

	// Insert at the tail (position == count)
	list.InsertAt(list.GetCount(), 999);
	Assert(list.Tail() == 999);
}

Fact("List RemoveAt Single")
{
	List<int> list;
	for (int i = 0; i < 10; i++)
		list.Add(i);

	list.RemoveAt(5);
	Assert(list.GetCount() == 9);
	Assert(list[4] == 4);
	Assert(list[5] == 6);
}

Fact("List RemoveAt Range")
{
	List<int> list;
	for (int i = 0; i < 10; i++)
		list.Add(i);

	// Remove indices 3,4,5,6
	list.RemoveAt(3, 4);
	Assert(list.GetCount() == 6);
	Assert(list[2] == 2);
	Assert(list[3] == 7);
}

Fact("List RemoveAt Range At End")
{
	List<int> list;
	for (int i = 0; i < 5; i++)
		list.Add(i);

	// Removing through to the end doesn't need to shuffle memory
	list.RemoveAt(2, 3);
	Assert(list.GetCount() == 2);
	Assert(list[0] == 0);
	Assert(list[1] == 1);
}

Fact("List Remove By Value")
{
	List<int> list;
	list.Add(10);
	list.Add(20);
	list.Add(30);

	Assert(list.Remove(20) == 1);
	Assert(list.GetCount() == 2);
	Assert(list[0] == 10);
	Assert(list[1] == 30);

	Assert(list.Remove(999) == -1);
	Assert(list.GetCount() == 2);
}

Fact("List DetachAt")
{
	List<int> list;
	list.Add(1);
	list.Add(2);
	list.Add(3);

	int val = list.DetachAt(1);
	Assert(val == 2);
	Assert(list.GetCount() == 2);
	Assert(list[0] == 1);
	Assert(list[1] == 3);
}

Fact("List Clear")
{
	List<int> list;
	list.Add(1);
	list.Add(2);
	list.Clear();
	Assert(list.GetCount() == 0);
	Assert(list.IsEmpty());

	// List remains usable after being cleared
	list.Add(5);
	Assert(list[0] == 5);
}

Fact("List Swap")
{
	List<int> list;
	for (int i = 0; i < 4; i++)
		list.Add(i);

	list.Swap(1, 2);
	Assert(list[0] == 0);
	Assert(list[1] == 2);
	Assert(list[2] == 1);
	Assert(list[3] == 3);

	// Swapping an item with itself is a no-op
	list.Swap(0, 0);
	Assert(list[0] == 0);
}

Fact("List Move Element")
{
	List<int> list;
	for (int i = 0; i < 4; i++)
		list.Add(i);

	list.Move(3, 1);
	Assert(list[0] == 0);
	Assert(list[1] == 3);
	Assert(list[2] == 1);
	Assert(list[3] == 2);

	list.Move(1, 3);
	Assert(list[0] == 0);
	Assert(list[1] == 1);
	Assert(list[2] == 2);
	Assert(list[3] == 3);
}

Fact("List ReplaceAt")
{
	List<int> list;
	list.Add(1);
	list.Add(2);
	list.Add(3);

	list.ReplaceAt(1, 99);
	Assert(list[0] == 1);
	Assert(list[1] == 99);
	Assert(list[2] == 3);
}

Fact("List SetCount")
{
	List<int> list;
	list.SetCount(5, 7);
	Assert(list.GetCount() == 5);
	for (int i = 0; i < 5; i++)
		Assert(list[i] == 7);

	list.SetCount(2, 0);
	Assert(list.GetCount() == 2);
	Assert(list[0] == 7);
	Assert(list[1] == 7);
}

Fact("List AddRange")
{
	List<int> a;
	a.Add(1);
	a.Add(2);
	a.Add(3);

	List<int> b;
	b.Add(4);
	b.Add(5);

	a.AddRange(b);
	Assert(a.GetCount() == 5);
	Assert(a[3] == 4);
	Assert(a[4] == 5);

	// Source list is unaffected
	Assert(b.GetCount() == 2);
}

Fact("List InsertRangeAt")
{
	List<int> a;
	a.Add(1);
	a.Add(2);
	a.Add(3);

	List<int> b;
	b.Add(10);
	b.Add(20);

	a.InsertRangeAt(1, b);
	Assert(a.GetCount() == 5);
	Assert(a[0] == 1);
	Assert(a[1] == 10);
	Assert(a[2] == 20);
	Assert(a[3] == 2);
	Assert(a[4] == 3);
}

Fact("List AddMany From List")
{
	List<int> src;
	src.Add(4);
	src.Add(5);

	List<int> dest;
	dest.Add(1);
	dest.Add(2);
	dest.Add(3);

	dest.AddMany(src);
	Assert(dest.GetCount() == 5);
	Assert(dest[0] == 1);
	Assert(dest[1] == 2);
	Assert(dest[2] == 3);
	Assert(dest[3] == 4);
	Assert(dest[4] == 5);
}

Fact("List AddMany From Set")
{
	Set<int> src;
	src.Add(10);
	src.Add(20);
	src.Add(30);

	List<int> dest;
	dest.Add(1);

	dest.AddMany(src);
	Assert(dest.GetCount() == 4);
	Assert(dest[0] == 1);

	// Set iteration order isn't insertion order - just check membership
	Assert(dest.Contains(10));
	Assert(dest.Contains(20));
	Assert(dest.Contains(30));
}

Fact("List AddMany Empty Source")
{
	List<int> src;
	List<int> dest;
	dest.Add(1);
	dest.Add(2);

	dest.AddMany(src);
	Assert(dest.GetCount() == 2);
	Assert(dest[0] == 1);
	Assert(dest[1] == 2);
}

Fact("List InsertManyAt From List")
{
	List<int> dest;
	dest.Add(1);
	dest.Add(2);
	dest.Add(3);

	List<int> src;
	src.Add(100);
	src.Add(101);

	dest.InsertManyAt(1, src);
	Assert(dest.GetCount() == 5);
	Assert(dest[0] == 1);
	Assert(dest[1] == 100);
	Assert(dest[2] == 101);
	Assert(dest[3] == 2);
	Assert(dest[4] == 3);
}

Fact("List InsertManyAt At Beginning")
{
	List<int> dest;
	dest.Add(1);
	dest.Add(2);

	List<int> src;
	src.Add(-1);
	src.Add(-2);

	dest.InsertManyAt(0, src);
	Assert(dest.GetCount() == 4);
	Assert(dest[0] == -1);
	Assert(dest[1] == -2);
	Assert(dest[2] == 1);
	Assert(dest[3] == 2);
}

Fact("List InsertManyAt At End")
{
	List<int> dest;
	dest.Add(1);
	dest.Add(2);

	List<int> src;
	src.Add(3);
	src.Add(4);

	dest.InsertManyAt(dest.GetCount(), src);
	Assert(dest.GetCount() == 4);
	Assert(dest[0] == 1);
	Assert(dest[1] == 2);
	Assert(dest[2] == 3);
	Assert(dest[3] == 4);
}

Fact("List InsertManyAt From Set")
{
	List<int> dest;
	dest.Add(1);
	dest.Add(2);

	Set<int> src;
	src.Add(50);
	src.Add(60);

	dest.InsertManyAt(1, src);
	Assert(dest.GetCount() == 4);
	Assert(dest[0] == 1);
	Assert(dest[3] == 2);

	// Set iteration order isn't insertion order - just check the two
	// inserted items landed in the middle, in some order
	Assert((dest[1] == 50 && dest[2] == 60) || (dest[1] == 60 && dest[2] == 50));
}

Fact("List InsertManyAt Empty Source")
{
	List<int> dest;
	dest.Add(1);
	dest.Add(2);

	List<int> src;
	dest.InsertManyAt(1, src);

	Assert(dest.GetCount() == 2);
	Assert(dest[0] == 1);
	Assert(dest[1] == 2);
}

Fact("List Filter")
{
	List<int> list;
	for (int i = 1; i <= 10; i++)
		list.Add(i);

	List<int> evens = list.Filter([](const int& v) { return v % 2 == 0; });
	Assert(evens.GetCount() == 5);
	Assert(evens[0] == 2);
	Assert(evens[1] == 4);
	Assert(evens[2] == 6);
	Assert(evens[3] == 8);
	Assert(evens[4] == 10);

	// Source list unaffected
	Assert(list.GetCount() == 10);
}

Fact("List Filter None Match")
{
	List<int> list;
	list.Add(1);
	list.Add(2);
	list.Add(3);

	List<int> result = list.Filter([](const int& v) { return v > 100; });
	Assert(result.GetCount() == 0);
	Assert(result.IsEmpty());
}

Fact("List Filter All Match")
{
	List<int> list;
	list.Add(1);
	list.Add(2);
	list.Add(3);

	List<int> result = list.Filter([](const int& v) { return true; });
	Assert(result.GetCount() == 3);
	Assert(result[0] == 1);
	Assert(result[1] == 2);
	Assert(result[2] == 3);
}

Fact("List Filter Empty List")
{
	List<int> list;
	List<int> result = list.Filter([](const int& v) { return v > 0; });
	Assert(result.GetCount() == 0);
	Assert(result.IsEmpty());
}

Fact("List Filter With Captured State")
{
	List<int> list;
	for (int i = 1; i <= 5; i++)
		list.Add(i);

	int threshold = 3;
	List<int> result = list.Filter([threshold](const int& v) { return v > threshold; });
	Assert(result.GetCount() == 2);
	Assert(result[0] == 4);
	Assert(result[1] == 5);
}

Fact("List Filter Of Strings")
{
	List<String> list;
	list.Add("Apples");
	list.Add("Pears");
	list.Add("Bananas");
	list.Add("Apricots");

	List<String> result = list.Filter([](const String& v) { return v.StartsWith("Ap"); });
	Assert(result.GetCount() == 2);
	Assert(result[0].IsEqualTo("Apples"));
	Assert(result[1].IsEqualTo("Apricots"));
}

Fact("List Map Same Type")
{
	List<int> list;
	list.Add(1);
	list.Add(2);
	list.Add(3);

	List<int> doubled = list.Map<int>([](const int& v) { return v * 2; });
	Assert(doubled.GetCount() == 3);
	Assert(doubled[0] == 2);
	Assert(doubled[1] == 4);
	Assert(doubled[2] == 6);

	// Source list unaffected
	Assert(list.GetCount() == 3);
	Assert(list[0] == 1);
}

Fact("List Map To Different Type")
{
	List<int> list;
	list.Add(1);
	list.Add(2);
	list.Add(3);

	List<String> strs = list.Map<String>([](const int& v) { return String::Format("#%d", v); });
	Assert(strs.GetCount() == 3);
	Assert(strs[0].IsEqualTo("#1"));
	Assert(strs[1].IsEqualTo("#2"));
	Assert(strs[2].IsEqualTo("#3"));
}

Fact("List Map Extracts Struct Field")
{
	struct Item
	{
		int id;
		const char* name;
	};

	List<Item> items;
	items.Add({ 1, "Apples" });
	items.Add({ 2, "Pears" });
	items.Add({ 3, "Bananas" });

	List<int> ids = items.Map<int>([](const Item& item) { return item.id; });
	Assert(ids.GetCount() == 3);
	Assert(ids[0] == 1);
	Assert(ids[1] == 2);
	Assert(ids[2] == 3);

	List<String> names = items.Map<String>([](const Item& item) { return String(item.name); });
	Assert(names.GetCount() == 3);
	Assert(names[0].IsEqualTo("Apples"));
	Assert(names[2].IsEqualTo("Bananas"));
}

Fact("List Map Empty List")
{
	List<int> list;
	List<String> result = list.Map<String>([](const int& v) { return String::Format("%d", v); });
	Assert(result.GetCount() == 0);
	Assert(result.IsEmpty());
}

Fact("List Map With Captured State")
{
	List<int> list;
	list.Add(1);
	list.Add(2);
	list.Add(3);

	int multiplier = 10;
	List<int> result = list.Map<int>([multiplier](const int& v) { return v * multiplier; });
	Assert(result.GetCount() == 3);
	Assert(result[0] == 10);
	Assert(result[1] == 20);
	Assert(result[2] == 30);
}

Fact("List FreeExtra")
{
	List<int> list;
	for (int i = 0; i < 20; i++)
		list.Add(i);

	list.RemoveAt(5, 15);
	Assert(list.GetCount() == 5);

	list.FreeExtra();
	Assert(list.GetCount() == 5);
	for (int i = 0; i < 5; i++)
		Assert(list[i] == i);
}

Fact("List Sort")
{
	List<int> list;
	list.Add(30);
	list.Add(10);
	list.Add(20);
	list.Add(5);

	list.Sort(CompareInts);
	Assert(list[0] == 5);
	Assert(list[1] == 10);
	Assert(list[2] == 20);
	Assert(list[3] == 30);
}

Fact("List Sort Default Compare")
{
	List<int> list;
	list.Add(30);
	list.Add(10);
	list.Add(20);
	list.Add(5);

	list.Sort();
	Assert(list[0] == 5);
	Assert(list[1] == 10);
	Assert(list[2] == 20);
	Assert(list[3] == 30);
}

Fact("List Sort Default Compare With Strings")
{
	List<String> list;
	list.Add(String("banana"));
	list.Add(String("apple"));
	list.Add(String("cherry"));

	list.Sort();
	Assert(list[0].IsEqualTo("apple"));
	Assert(list[1].IsEqualTo("banana"));
	Assert(list[2].IsEqualTo("cherry"));
}

Fact("List Sort With Context")
{
	List<int> list;
	list.Add(1);
	list.Add(3);
	list.Add(2);

	int iSign = -1;
	list.Sort(CompareIntsWithContext, &iSign);
	Assert(list[0] == 3);
	Assert(list[1] == 2);
	Assert(list[2] == 1);
}

Fact("List Stack Operations")
{
	List<int> list;
	list.Push(10);
	list.Push(20);
	list.Push(30);
	Assert(list.GetCount() == 3);
	Assert(list[0] == 10);
	Assert(list[1] == 20);
	Assert(list[2] == 30);
	Assert(list.Tail() == 30);
	Assert(list.Pop() == 30);
	Assert(list.Pop() == 20);
	Assert(list.Pop() == 10);
	Assert(list.IsEmpty());
}

Fact("List Queue Operations")
{
	List<int> list;
	list.Enqueue(10);
	list.Enqueue(20);
	list.Enqueue(30);
	Assert(list.GetCount() == 3);
	Assert(list[0] == 10);
	Assert(list[1] == 20);
	Assert(list[2] == 30);
	Assert(list.Head() == 10);
	Assert(list.Dequeue() == 10);
	Assert(list.Dequeue() == 20);
	Assert(list.Dequeue() == 30);
	Assert(list.IsEmpty());
}

Fact("List Try Methods On Empty List")
{
	List<int> list;
	int val = -1;
	Assert(!list.TryPop(val));
	Assert(!list.TryTail(val));
	Assert(!list.TryHead(val));
	Assert(!list.TryDequeue(val));
}

Fact("List Try Methods On Non-Empty List")
{
	List<int> list;
	list.Add(42);

	int val = -1;
	Assert(list.TryTail(val) && val == 42);
	Assert(list.TryHead(val) && val == 42);
	Assert(list.TryPop(val) && val == 42);
	Assert(list.IsEmpty());
}

Fact("List Move Constructor")
{
	List<int> a;
	a.Add(1);
	a.Add(2);
	a.Add(3);

	List<int> b(move(a));
	Assert(b.GetCount() == 3);
	Assert(b[0] == 1);
	Assert(b[1] == 2);
	Assert(b[2] == 3);

	// Source list is left empty
	Assert(a.GetCount() == 0);
	Assert(a.GetBuffer() == nullptr);
}

Fact("List Move Assignment")
{
	List<int> a;
	a.Add(1);
	a.Add(2);

	List<int> b;
	b.Add(99);

	b = move(a);
	Assert(b.GetCount() == 2);
	Assert(b[0] == 1);
	Assert(b[1] == 2);
	Assert(a.GetCount() == 0);
}

Fact("List Construction And Destruction Of Elements")
{
	InstanceCounter::s_iInstances = 0;
	{
		List<InstanceCounter> list;
		list.Add(InstanceCounter());
		list.Add(InstanceCounter());
		list.Add(InstanceCounter());
		Assert(InstanceCounter::s_iInstances == 3);

		list.RemoveAt(0);
		Assert(InstanceCounter::s_iInstances == 2);

		list.Clear();
		Assert(InstanceCounter::s_iInstances == 0);
	}
	Assert(InstanceCounter::s_iInstances == 0);
}

Fact("List Of Owned Pointers")
{
	InstanceCounter::s_iInstances = 0;

	List<OwnedPtr<InstanceCounter>> list;
	list.Add(new InstanceCounter());
	list.Add(new InstanceCounter());
	list.Add(new InstanceCounter());
	list.Add(new InstanceCounter());
	Assert(InstanceCounter::s_iInstances == 4);

	// Removing an owned pointer deletes the object it owns
	list.RemoveAt(0);
	Assert(InstanceCounter::s_iInstances == 3);

	// Detaching transfers ownership out of the list without deleting
	OwnedPtr<InstanceCounter> op = list.DetachAt(0);
	Assert(InstanceCounter::s_iInstances == 3);
	InstanceCounter* p = op.Detach();
	Assert(InstanceCounter::s_iInstances == 3);
	delete p;
	Assert(InstanceCounter::s_iInstances == 2);

	list.Clear();
	Assert(InstanceCounter::s_iInstances == 0);
}

Fact("List Of Owned Pointers Indexed Access")
{
	InstanceCounter::s_iInstances = 0;

	List<OwnedPtr<InstanceCounter>> list;
	list.Add(new InstanceCounter(10));
	list.Add(new InstanceCounter(20));
	list.Add(new InstanceCounter(30));

	// operator[]/GetAt must return a reference to the stored OwnedPtr so
	// -> and * work directly on the element
	Assert(list[0]->Value == 10);
	Assert(list[1]->Value == 20);
	Assert(list.GetAt(2)->Value == 30);

	list[0]->Value = 100;
	Assert(list[0]->Value == 100);

	list.Clear();
	Assert(InstanceCounter::s_iInstances == 0);
}

Fact("List Of Owned Pointers Releases On Destruction")
{
	InstanceCounter::s_iInstances = 0;
	{
		List<OwnedPtr<InstanceCounter>> list;
		list.Add(new InstanceCounter());
		list.Add(new InstanceCounter());
	}
	Assert(InstanceCounter::s_iInstances == 0);
}

Fact("List Of Shared Pointers")
{
	InstanceCounter::s_iInstances = 0;

	List<SharedPtr<InstanceCounter>> list;
	list.Add(new InstanceCounter());
	list.Add(new InstanceCounter());
	list.Add(new InstanceCounter());
	Assert(InstanceCounter::s_iInstances == 3);

	{
		// Holding a copy of the shared pointer keeps the instance alive
		// even after it's removed from the list
		SharedPtr<InstanceCounter> kept = list[0];
		list.RemoveAt(0);
		Assert(InstanceCounter::s_iInstances == 3);
	}
	Assert(InstanceCounter::s_iInstances == 2);

	list.Clear();
	Assert(InstanceCounter::s_iInstances == 0);
}

Fact("List Of Strings")
{
	List<WString> list;
	list.Add(L"Apples");
	list.Add(L"Pears");
	list.Add(L"Bananas");

	Assert(list.GetCount() == 3);
	Assert(list.IndexOf(L"Pears") == 1);
	Assert(list.IndexOf<SCaseI>(L"PEARS") == 1);
	Assert(list.Contains(L"Bananas"));
	Assert(!list.Contains(L"Oranges"));
}

Fact("List Iterate")
{
	List<int> list;
	list.Add(1);
	list.Add(2);
	list.Add(3);

	List<int> seen;
	for (auto it = list.Iterate(); it.Next(); )
		seen.Add(it.Get());

	Assert(seen.GetCount() == 3);
	Assert(seen[0] == 1);
	Assert(seen[1] == 2);
	Assert(seen[2] == 3);
}

Fact("List IterateReverse")
{
	List<int> list;
	list.Add(1);
	list.Add(2);
	list.Add(3);

	List<int> seen;
	for (auto it = list.IterateReverse(); it.Next(); )
		seen.Add(it.Get());

	Assert(seen.GetCount() == 3);
	Assert(seen[0] == 3);
	Assert(seen[1] == 2);
	Assert(seen[2] == 1);
}

Fact("List IterateReverse Empty List")
{
	List<int> list;
	auto it = list.IterateReverse();
	Assert(!it.Next());
}
