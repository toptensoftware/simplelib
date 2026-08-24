#include "../UnitTesting.h"
#include "../Core.h"
using namespace SimpleLib;

Fact("Nullable Default Constructor Has No Value")
{
	Nullable<int> n;
	Assert(!n.HasValue());
}

Fact("Nullable Value Constructor Has Value")
{
	Nullable<int> n(42);
	Assert(n.HasValue());
	Assert(n.GetValue() == 42);
}

Fact("Nullable Operator Assign From T Sets Value")
{
	Nullable<int> n;
	n = 5;
	Assert(n.HasValue());
	Assert(n.GetValue() == 5);
}

Fact("Nullable Operator Assign From T Overwrites Existing Value")
{
	Nullable<int> n(1);
	n = 2;
	Assert(n.HasValue());
	Assert(n.GetValue() == 2);
}

Fact("Nullable Copy Constructor Copies Value")
{
	Nullable<int> a(7);
	Nullable<int> b(a);
	Assert(b.HasValue());
	Assert(b.GetValue() == 7);
}

Fact("Nullable Copy Constructor Copies Null State")
{
	Nullable<int> a;
	Nullable<int> b(a);
	Assert(!b.HasValue());
}

Fact("Nullable Copy Assignment Copies Value")
{
	Nullable<int> a(9);
	Nullable<int> b;
	b = a;
	Assert(b.HasValue());
	Assert(b.GetValue() == 9);
}

Fact("Nullable Copy Assignment Copies Null State")
{
	Nullable<int> a(9);
	Nullable<int> b;
	a = b;
	Assert(!a.HasValue());
}

Fact("Nullable Clear Resets To Null")
{
	Nullable<int> n(3);
	n.Clear();
	Assert(!n.HasValue());
}

Fact("Nullable Clear Then Set Works")
{
	Nullable<int> n(3);
	n.Clear();
	n = 4;
	Assert(n.HasValue());
	Assert(n.GetValue() == 4);
}

Fact("Nullable Works For Non Pod Types")
{
	// Clear() used to do `m_value = { 0 }` which is undefined behaviour
	// for a non-POD type like String (0 implicitly converts to a null
	// const char*, which String's constructor then dereferences).
	Nullable<String> n(String("hello"));
	Assert(n.HasValue());
	Assert(n.GetValue().IsEqualTo("hello"));

	n.Clear();
	Assert(!n.HasValue());

	n = String("world");
	Assert(n.HasValue());
	Assert(n.GetValue().IsEqualTo("world"));
}

Fact("Nullable Default Constructed Non Pod Type Has No Value")
{
	Nullable<String> n;
	Assert(!n.HasValue());
}
