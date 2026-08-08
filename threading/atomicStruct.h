#pragma once

#include "atomic.h"

namespace SimpleLib
{

// Implements an atomic way of accessing a structure by keeping
// two copies and atomically switching the pointer that references
// the current one.
template <class T>
class AtomicStruct
{
public:
	AtomicStruct(const T& initial)
	{
		a = initial;
		current.Set(&a);
	}

	void Get(T& out)
	{
		out = *current.Get();
	}

	void Set(const T& in)
	{
		if (current.Get() == &a)
		{
			b = in;
			current.Set(&b);
		}
		else
		{
			a = in;
			current.Set(&a);
		}
	}

	Atomic<T*> current;
	T a;
	T b;
};



}