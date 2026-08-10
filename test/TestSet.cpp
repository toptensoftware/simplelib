#include "../UnitTesting.h"
#include "../Core.h"
using namespace SimpleLib;

// A hashable/comparable element that also tracks live instances, so we can
// verify the set correctly constructs and destroys elements as they're
// added, replaced, removed and cleared.
class TrackedValue
{
public:
	TrackedValue(int val = 0) : Value(val) { s_iInstances++; }
	TrackedValue(const TrackedValue& other) : Value(other.Value) { s_iInstances++; }
	~TrackedValue() { s_iInstances--; }

	bool operator==(const TrackedValue& other) const { return Value == other.Value; }

	static uint32_t Hash(const TrackedValue& v) { return SDefaultCompare::Hash(v.Value); }

	int Value;
	inline static int s_iInstances = 0;
};

Fact("Set Basics")
{
	Set<int> set;
	set.Add(10);
	set.Add(20);
	set.Add(30);
	Assert(set.GetCount() == 3);
	Assert(set.Contains(10));
	Assert(set.Contains(20));
	Assert(set.Contains(30));
}

Fact("Set Empty By Default")
{
	Set<int> set;
	Assert(set.GetCount() == 0);
	Assert(set.IsEmpty());
}

Fact("Set Add Duplicate Does Not Increase Count")
{
	Set<int> set;
	set.Add(1);
	set.Add(1);
	set.Add(1);
	Assert(set.GetCount() == 1);
	Assert(set.Contains(1));
}

Fact("Set AddMany From Set")
{
	Set<int> src;
	src.Add(1);
	src.Add(2);
	src.Add(3);

	Set<int> dest;
	dest.Add(3);
	dest.Add(4);

	dest.AddMany(src);
	Assert(dest.GetCount() == 4);	// {1,2,3,4} - duplicate 3 collapses
	Assert(dest.Contains(1));
	Assert(dest.Contains(2));
	Assert(dest.Contains(3));
	Assert(dest.Contains(4));
}

Fact("Set AddMany From List With Duplicates")
{
	List<int> src;
	src.Add(1);
	src.Add(2);
	src.Add(2);
	src.Add(3);

	Set<int> dest;
	dest.AddMany(src);

	Assert(dest.GetCount() == 3);
	Assert(dest.Contains(1));
	Assert(dest.Contains(2));
	Assert(dest.Contains(3));
}

Fact("Set AddMany Empty Source")
{
	List<int> src;
	Set<int> dest;
	dest.Add(1);

	dest.AddMany(src);
	Assert(dest.GetCount() == 1);
	Assert(dest.Contains(1));
}

Fact("Set Remove")
{
	Set<int> set;
	set.Add(1);
	set.Add(2);
	Assert(set.Remove(1));
	Assert(set.GetCount() == 1);
	Assert(!set.Contains(1));
	Assert(set.Contains(2));
}

Fact("Set Remove Missing Returns False")
{
	Set<int> set;
	set.Add(1);
	Assert(!set.Remove(999));
	Assert(set.GetCount() == 1);
}

Fact("Set Clear")
{
	Set<int> set;
	set.Add(1);
	set.Add(2);
	set.Clear();
	Assert(set.GetCount() == 0);
	Assert(set.IsEmpty());

	// Set remains usable after being cleared
	set.Add(3);
	Assert(set.GetCount() == 1);
	Assert(set.Contains(3));
}

Fact("Set Contains")
{
	Set<int> set;
	set.Add(1);
	Assert(set.Contains(1));
	Assert(!set.Contains(999));
}

Fact("Set Iterate")
{
	Set<int> set;
	set.Add(1);
	set.Add(2);
	set.Add(3);

	int iSeen = 0;
	int iSum = 0;
	auto it = set.Iterate();
	while (it.Next())
	{
		iSeen++;
		iSum += it.Get();
	}
	Assert(iSeen == 3);
	Assert(iSum == 6);
}

Fact("Set IterateReverse")
{
	Set<int> set;
	set.Add(1);
	set.Add(2);
	set.Add(3);

	int iSeen = 0;
	int iSum = 0;
	auto it = set.IterateReverse();
	while (it.Next())
	{
		iSeen++;
		iSum += it.Get();
	}
	Assert(iSeen == 3);
	Assert(iSum == 6);
}

Fact("Set IterateReverse Is Exact Reverse Of Iterate")
{
	Set<int> set;
	for (int i = 0; i < 50; i++)
		set.Add(i);

	List<int> forward;
	for (auto it = set.Iterate(); it.Next(); )
		forward.Add(it.Get());

	List<int> reverse;
	for (auto it = set.IterateReverse(); it.Next(); )
		reverse.Add(it.Get());

	Assert(forward.GetCount() == 50);
	Assert(reverse.GetCount() == 50);
	for (int i = 0; i < 50; i++)
		Assert(forward[i] == reverse[49 - i]);
}

Fact("Set IterateReverse Empty Set")
{
	Set<int> set;
	auto it = set.IterateReverse();
	Assert(!it.Next());
}

Fact("Set Many Elements")
{
	Set<int> set;
	for (int i = 0; i < 200; i++)
		set.Add(i);

	Assert(set.GetCount() == 200);
	for (int i = 0; i < 200; i++)
		Assert(set.Contains(i));

	for (int i = 0; i < 200; i += 2)
		Assert(set.Remove(i));

	Assert(set.GetCount() == 100);
	for (int i = 0; i < 200; i++)
		Assert(set.Contains(i) == (i % 2 != 0));
}

Fact("Set Move Constructor")
{
	Set<int> a;
	a.Add(1);
	a.Add(2);

	Set<int> b(move(a));
	Assert(b.GetCount() == 2);
	Assert(b.Contains(1));
	Assert(b.Contains(2));
	Assert(a.GetCount() == 0);
}

Fact("Set Move Assignment")
{
	Set<int> a;
	a.Add(1);

	Set<int> b;
	b.Add(99);

	b = move(a);
	Assert(b.GetCount() == 1);
	Assert(b.Contains(1));
	Assert(a.GetCount() == 0);
}

Fact("Set Construction And Destruction Of Elements")
{
	TrackedValue::s_iInstances = 0;
	{
		Set<TrackedValue> set;
		set.Add(TrackedValue(1));
		set.Add(TrackedValue(2));
		set.Add(TrackedValue(3));
		Assert(TrackedValue::s_iInstances == 3);

		set.Remove(TrackedValue(1));
		Assert(TrackedValue::s_iInstances == 2);

		set.Clear();
		Assert(TrackedValue::s_iInstances == 0);
	}
	Assert(TrackedValue::s_iInstances == 0);
}

Fact("Set Add Duplicate Destroys Old Value")
{
	TrackedValue::s_iInstances = 0;
	{
		Set<TrackedValue> set;
		set.Add(TrackedValue(1));
		Assert(TrackedValue::s_iInstances == 1);

		// Adding an element equal to one already present must destroy the
		// old one before storing the new one, not leak it
		set.Add(TrackedValue(1));
		Assert(TrackedValue::s_iInstances == 1);
		Assert(set.GetCount() == 1);
	}
	Assert(TrackedValue::s_iInstances == 0);
}

Fact("Set Of Strings")
{
	Set<String> set;
	set.Add("Apples");
	set.Add("Pears");
	set.Add("Bananas");

	Assert(set.GetCount() == 3);
	Assert(set.Contains("Pears"));
	Assert(!set.Contains("Oranges"));

	set.Remove("Pears");
	Assert(set.GetCount() == 2);
	Assert(!set.Contains("Pears"));
}

Fact("Set Union")
{
	Set<int> a;
	a.Add(1);
	a.Add(2);
	a.Add(3);
	a.Add(4);

	Set<int> b;
	b.Add(3);
	b.Add(4);
	b.Add(5);
	b.Add(6);

	Set<int> u = Set<int>::Union(a, b);
	Assert(u.GetCount() == 6);
	Assert(u.Contains(1));
	Assert(u.Contains(2));
	Assert(u.Contains(3));
	Assert(u.Contains(4));
	Assert(u.Contains(5));
	Assert(u.Contains(6));

	// Sources unaffected
	Assert(a.GetCount() == 4);
	Assert(b.GetCount() == 4);
}

Fact("Set Union With Empty Set")
{
	Set<int> a;
	a.Add(1);
	a.Add(2);

	Set<int> empty;

	Set<int> u1 = Set<int>::Union(a, empty);
	Assert(u1.GetCount() == 2);
	Assert(u1.Contains(1));
	Assert(u1.Contains(2));

	Set<int> u2 = Set<int>::Union(empty, empty);
	Assert(u2.IsEmpty());
}

Fact("Set Intersection")
{
	Set<int> a;
	a.Add(1);
	a.Add(2);
	a.Add(3);
	a.Add(4);

	Set<int> b;
	b.Add(3);
	b.Add(4);
	b.Add(5);
	b.Add(6);

	Set<int> i = Set<int>::Intersection(a, b);
	Assert(i.GetCount() == 2);
	Assert(i.Contains(3));
	Assert(i.Contains(4));
	Assert(!i.Contains(1));
	Assert(!i.Contains(5));

	// Sources unaffected
	Assert(a.GetCount() == 4);
	Assert(b.GetCount() == 4);
}

Fact("Set Intersection No Overlap")
{
	Set<int> a;
	a.Add(1);
	a.Add(2);

	Set<int> b;
	b.Add(3);
	b.Add(4);

	Set<int> i = Set<int>::Intersection(a, b);
	Assert(i.IsEmpty());
}

Fact("Set Intersection With Empty Set")
{
	Set<int> a;
	a.Add(1);
	a.Add(2);

	Set<int> empty;
	Set<int> i = Set<int>::Intersection(a, empty);
	Assert(i.IsEmpty());
}

Fact("Set Difference")
{
	Set<int> a;
	a.Add(1);
	a.Add(2);
	a.Add(3);
	a.Add(4);

	Set<int> b;
	b.Add(3);
	b.Add(4);
	b.Add(5);
	b.Add(6);

	// a - b
	Set<int> d1 = Set<int>::Difference(a, b);
	Assert(d1.GetCount() == 2);
	Assert(d1.Contains(1));
	Assert(d1.Contains(2));
	Assert(!d1.Contains(3));
	Assert(!d1.Contains(4));

	// Not symmetric: b - a
	Set<int> d2 = Set<int>::Difference(b, a);
	Assert(d2.GetCount() == 2);
	Assert(d2.Contains(5));
	Assert(d2.Contains(6));

	// Sources unaffected
	Assert(a.GetCount() == 4);
	Assert(b.GetCount() == 4);
}

Fact("Set Difference With Empty Set")
{
	Set<int> a;
	a.Add(1);
	a.Add(2);

	Set<int> empty;

	Set<int> d1 = Set<int>::Difference(a, empty);
	Assert(d1.GetCount() == 2);
	Assert(d1.Contains(1));
	Assert(d1.Contains(2));

	Set<int> d2 = Set<int>::Difference(empty, a);
	Assert(d2.IsEmpty());
}

Fact("Set Difference Identical Sets Is Empty")
{
	Set<int> a;
	a.Add(1);
	a.Add(2);

	Set<int> b;
	b.Add(1);
	b.Add(2);

	Set<int> d = Set<int>::Difference(a, b);
	Assert(d.IsEmpty());
}
