#pragma once

#include "Atomic.h"

namespace SimpleLib
{

// Implements an atomic way of accessing a structure using a seqlock:
// Set() bumps a sequence counter to odd, writes the value, then bumps
// it back to even. Get() reads the counter, copies the value, then
// reads the counter again and retries if it changed (or was odd to
// begin with) - ie: if a writer raced it. The writer never blocks or
// spins - only Get() can spin, and only while actually racing a Set().
template <class T>
class AtomicStruct
{
public:
	AtomicStruct(const T& initial) : value(initial)
	{
	}

	void Get(T& out)
	{
		for (;;)
		{
			uint32_t seq1 = sequence.Get();
			if (seq1 & 1)
				continue;

			out = value;

			uint32_t seq2 = sequence.Get();
			if (seq1 == seq2)
				return;
		}
	}

	void Set(const T& in)
	{
		sequence.Add(1);
		value = in;
		sequence.Add(1);
	}

	Atomic<uint32_t> sequence;
	T value;
};



}