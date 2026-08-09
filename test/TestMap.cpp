#include "../UnitTesting.h"
#include "../Core.h"
using namespace SimpleLib;

// Tracks live instances so we can verify the map correctly constructs and
// destroys elements as they're added, removed, replaced and cleared.
class InstanceCounter
{
public:
	InstanceCounter() { s_iInstances++; }
	InstanceCounter(const InstanceCounter&) { s_iInstances++; }
	~InstanceCounter() { s_iInstances--; }

	inline static int s_iInstances = 0;
};

Fact("Map Basics")
{
	Map<int, int> map;
	map.Add(1, 100);
	map.Add(2, 200);
	map.Add(3, 300);
	Assert(map.GetCount() == 3);
	Assert(map.Get(1) == 100);
	Assert(map.Get(2) == 200);
	Assert(map.Get(3) == 300);
}

Fact("Map Empty By Default")
{
	Map<int, int> map;
	Assert(map.GetCount() == 0);
	Assert(map.IsEmpty());
}

Fact("Map Add Does Not Replace Existing Key")
{
	Map<int, int> map;
	map.Add(1, 100);
	map.Add(1, 200);	// Key already exists, should be a no-op
	Assert(map.GetCount() == 1);
	Assert(map.Get(1) == 100);
}

Fact("Map Set Adds New Key")
{
	Map<int, int> map;
	map.Set(1, 100);
	Assert(map.GetCount() == 1);
	Assert(map.Get(1) == 100);
}

Fact("Map Set Replaces Existing Key")
{
	Map<int, int> map;
	map.Set(1, 100);
	map.Set(1, 200);
	Assert(map.GetCount() == 1);
	Assert(map.Get(1) == 200);
}

Fact("Map Remove")
{
	Map<int, int> map;
	map.Add(1, 100);
	map.Add(2, 200);
	Assert(map.Remove(1));
	Assert(map.GetCount() == 1);
	Assert(!map.ContainsKey(1));
	Assert(map.ContainsKey(2));
}

Fact("Map Remove Missing Key Returns False")
{
	Map<int, int> map;
	map.Add(1, 100);
	Assert(!map.Remove(999));
	Assert(map.GetCount() == 1);
}

Fact("Map Clear")
{
	Map<int, int> map;
	map.Add(1, 100);
	map.Add(2, 200);
	map.Clear();
	Assert(map.GetCount() == 0);
	Assert(map.IsEmpty());

	// Map remains usable after being cleared
	map.Add(3, 300);
	Assert(map.GetCount() == 1);
	Assert(map.Get(3) == 300);
}

Fact("Map Get With Default")
{
	Map<int, int> map;
	map.Add(1, 100);
	Assert(map.Get(1, -1) == 100);
	Assert(map.Get(999, -1) == -1);
}

Fact("Map TryGetValue")
{
	Map<int, int> map;
	map.Add(1, 100);

	int val = -1;
	Assert(map.TryGetValue(1, val));
	Assert(val == 100);

	Assert(!map.TryGetValue(999, val));
}

Fact("Map ContainsKey")
{
	Map<int, int> map;
	map.Add(1, 100);
	Assert(map.ContainsKey(1));
	Assert(!map.ContainsKey(999));
}

Fact("Map operator[]")
{
	Map<int, int> map;
	map.Add(1, 111);
	map.Add(2, 222);
	Assert(map[1] == 111);
	Assert(map[2] == 222);
}

Fact("Map Iterate")
{
	Map<int, int> map;
	map.Add(1, 100);
	map.Add(2, 200);
	map.Add(3, 300);

	int iSeen = 0;
	int iSum = 0;
	auto it = map.Iterate();
	while (it.Next())
	{
		iSeen++;
		iSum += it.GetValue();
		Assert(map.Get(it.GetKey()) == it.GetValue());
	}
	Assert(iSeen == 3);
	Assert(iSum == 600);
}

Fact("Map Many Keys")
{
	Map<int, int> map;
	for (int i = 0; i < 200; i++)
		map.Add(i, i * 10);

	Assert(map.GetCount() == 200);
	for (int i = 0; i < 200; i++)
		Assert(map.Get(i) == i * 10);

	for (int i = 0; i < 200; i += 2)
		Assert(map.Remove(i));

	Assert(map.GetCount() == 100);
	for (int i = 0; i < 200; i++)
	{
		if (i % 2 == 0)
			Assert(!map.ContainsKey(i));
		else
			Assert(map.Get(i) == i * 10);
	}
}

Fact("Map Move Constructor")
{
	Map<int, int> a;
	a.Add(1, 100);
	a.Add(2, 200);

	Map<int, int> b(move(a));
	Assert(b.GetCount() == 2);
	Assert(b.Get(1) == 100);
	Assert(b.Get(2) == 200);
	Assert(a.GetCount() == 0);
}

Fact("Map Move Assignment")
{
	Map<int, int> a;
	a.Add(1, 100);

	Map<int, int> b;
	b.Add(99, 999);

	b = move(a);
	Assert(b.GetCount() == 1);
	Assert(b.Get(1) == 100);
	Assert(a.GetCount() == 0);
}

Fact("Map Construction And Destruction Of Elements")
{
	InstanceCounter::s_iInstances = 0;
	{
		Map<int, InstanceCounter> map;
		map.Add(1, InstanceCounter());
		map.Add(2, InstanceCounter());
		map.Add(3, InstanceCounter());
		Assert(InstanceCounter::s_iInstances == 3);

		map.Remove(1);
		Assert(InstanceCounter::s_iInstances == 2);

		map.Clear();
		Assert(InstanceCounter::s_iInstances == 0);
	}
	Assert(InstanceCounter::s_iInstances == 0);
}

Fact("Map Set Replace Destroys Old Value")
{
	InstanceCounter::s_iInstances = 0;
	{
		Map<int, InstanceCounter> map;
		map.Set(1, InstanceCounter());
		Assert(InstanceCounter::s_iInstances == 1);

		// Replacing the value for an existing key must destroy the old one
		map.Set(1, InstanceCounter());
		Assert(InstanceCounter::s_iInstances == 1);
	}
	Assert(InstanceCounter::s_iInstances == 0);
}

Fact("Map TryGetValue Overwrites Existing Target Without Leaking")
{
	InstanceCounter::s_iInstances = 0;
	{
		Map<int, InstanceCounter> map;
		map.Add(1, InstanceCounter());
		Assert(InstanceCounter::s_iInstances == 1);

		{
			InstanceCounter target;
			Assert(InstanceCounter::s_iInstances == 2);

			// Overwriting an already-constructed target must destroy its
			// previous value first, not just construct over the top of it
			Assert(map.TryGetValue(1, target));
			Assert(InstanceCounter::s_iInstances == 2);
		}
		Assert(InstanceCounter::s_iInstances == 1);
	}
	Assert(InstanceCounter::s_iInstances == 0);
}

Fact("Map Of Owned Pointers")
{
	InstanceCounter::s_iInstances = 0;
	{
		Map<int, OwnedPtr<InstanceCounter>> map;
		map.Add(1, new InstanceCounter());
		map.Add(2, new InstanceCounter());
		Assert(InstanceCounter::s_iInstances == 2);

		map.Remove(1);
		Assert(InstanceCounter::s_iInstances == 1);
	}
	Assert(InstanceCounter::s_iInstances == 0);
}

Fact("Map Of String Keys")
{
	Map<String<char>, int> map;
	map.Add("Apples", 1);
	map.Add("Pears", 2);
	map.Add("Bananas", 3);

	Assert(map.GetCount() == 3);
	Assert(map.Get("Pears") == 2);
	Assert(map.ContainsKey("Bananas"));
	Assert(!map.ContainsKey("Oranges"));
}
