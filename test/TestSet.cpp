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
	Set<String<char>> set;
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
