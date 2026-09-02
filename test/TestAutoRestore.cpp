#include <type_traits>

#include "../UnitTesting.h"
#include "../Core.h"
using namespace SimpleLib;

Fact("AutoRestore Restores Value On Scope Exit")
{
	int x = 10;
	{
		AutoRestore<int> ar(x, 20);
		Assert(x == 20);
	}
	Assert(x == 10);
}

Fact("AutoRestore Capture Only Constructor Leaves Value Unchanged")
{
	int x = 10;
	{
		AutoRestore<int> ar(x);
		Assert(x == 10);
		x = 99;
	}
	Assert(x == 10);
}

Fact("AutoRestore Restores Even After Further Modification")
{
	int x = 1;
	{
		AutoRestore<int> ar(x, 2);
		x = 3;
		x = 4;
	}
	Assert(x == 1);
}

Fact("AutoRestore Nested Scopes Restore In Order")
{
	int x = 0;
	{
		AutoRestore<int> a(x, 1);
		Assert(x == 1);
		{
			AutoRestore<int> b(x, 2);
			Assert(x == 2);
		}
		Assert(x == 1);
	}
	Assert(x == 0);
}

Fact("AutoRestore OldValue Returns Captured Value")
{
	int x = 42;
	AutoRestore<int> ar(x, 7);
	Assert(ar.OldValue() == 42);
	Assert(x == 7);
}

Fact("AutoRestore Works For Non Pod Types")
{
	String s("hello");
	{
		AutoRestore<String> ar(s, String("world"));
		Assert(s.IsEqualTo("world"));
	}
	Assert(s.IsEqualTo("hello"));
}

Fact("AutoRestore Is Not Copyable Or Movable")
{
	static_assert(!std::is_copy_constructible<AutoRestore<int>>::value,
		"AutoRestore must not be copy constructible");
	static_assert(!std::is_move_constructible<AutoRestore<int>>::value,
		"AutoRestore must not be move constructible");
	static_assert(!std::is_copy_assignable<AutoRestore<int>>::value,
		"AutoRestore must not be copy assignable");
	static_assert(!std::is_move_assignable<AutoRestore<int>>::value,
		"AutoRestore must not be move assignable");
}

Fact("AutoRestore Bool Flag Override Pattern")
{
	bool reentrant = false;
	{
		AutoRestore<bool> guard(reentrant, true);
		Assert(reentrant);
	}
	Assert(!reentrant);
}
