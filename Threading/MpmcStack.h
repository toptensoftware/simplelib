#pragma once

#include "Atomic.h"

namespace SimpleLib
{

// Implements an intrusive multi-producer, multi-consumer unbound lifo stack
// Type `T` must have a member `T* next`
//
// The head is a tagged pointer (pointer + generation counter, CAS'd
// together via 128-bit CAS) rather than a plain pointer, to avoid the ABA
// problem: under high contention with few distinct nodes (the same handful
// of nodes popped and pushed back repeatedly by many threads), a thread
// can be delayed between reading a node and CASing it out, during which
// another thread pops *and re-pushes* that same node - a plain pointer CAS
// can't distinguish that from the original, and can succeed spuriously,
// handing the same node out to two callers at once. The tag increments on
// every successful CAS, so a stale compare can never match again even if
// the pointer value cycles back.
template <class T>
class alignas(kCacheLineSize) MpmcStack
{
	using Head = AtomicTaggedPtr<T>;
	using HeadValue = typename Head::Value;

public:
	// Constructor
	MpmcStack()
	{
	}

	// Destructor
	virtual ~MpmcStack()
	{
	}

	// Reset the list
	void Reset()
	{
		m_pHead.Set(HeadValue{ nullptr, 0 });
		m_count.Set(0);
	}

	// Get the approximate count
	int GetLikelyCount()
	{
		return m_count.Get();
	}

	// Pushes an item onto the stack
	// Returns true if list transitioned from empty to not empty
	bool Push(T* item)
	{
		assert(item != nullptr);
		assert(item->next == nullptr);

		while (true)
		{
			HeadValue oldHead = m_pHead.Get();
			item->next = oldHead.ptr;
			bool isFirst = oldHead.ptr == nullptr;
			if (m_pHead.TrySet(HeadValue{ item, oldHead.tag + 1 }, oldHead))
			{
				m_count.Inc();
				return isFirst;
			}
		}
	}

	// Push multiple items onto the stack
	// The chain must be valid
	// Returns true if list transitioned from empty to not empty
	bool Push(T* first, T* last)
	{
		assert(first != nullptr);

		// Count items (and check chain)
		int count = 0;
		T* p = first;
		T* prev = nullptr;
		while (p)
		{
			count++;
			prev = p;
			p = p->next;
		}

		assert(prev == last);

		while(true)
		{
			HeadValue oldHead = m_pHead.Get();
			last->next = oldHead.ptr;
			bool isFirst = oldHead.ptr == nullptr;
			if (m_pHead.TrySet(HeadValue{ first, oldHead.tag + 1 }, oldHead))
			{
				m_count.Add(count);
				return isFirst;
			}
		}
	}

	bool PushMany(T* first)
	{
		assert(first != nullptr);

		int count = 0;
		T* p = first;
		T* last = nullptr;
		while (p)
		{
			count++;
			last = p;
			p = p->next;
		}

		while(true)
		{
			HeadValue oldHead = m_pHead.Get();
			last->next = oldHead.ptr;
			bool isFirst = oldHead.ptr == nullptr;
			if (m_pHead.TrySet(HeadValue{ first, oldHead.tag + 1 }, oldHead))
			{
				m_count.Add(count);
				return isFirst;
			}
		}
	}

	// Pops item from the stack
	// Returns nullptr if the list is empty
	// Returns nowEmpty true if the list transitioned to empty
	T* Pop(bool& nowEmpty)
	{
		while (true)
		{
			// Get the popped item
			HeadValue oldHead = m_pHead.Get();
			T* item = oldHead.ptr;

			// Empty list?
			if (item == nullptr)
				return item;

			// Update head
			if (m_pHead.TrySet(HeadValue{ item->next, oldHead.tag + 1 }, oldHead))
			{
				nowEmpty = item->next == nullptr;
				item->next = nullptr;
				m_count.Dec();
				return item;
			}
		}

	}

	// Pops single item from the stack
	// Returns nullptr if the list was empty
	T* Pop()
	{
		bool unused;
		return Pop(unused);
	}

	// Pop all items from the stack
	T* PopAll()
	{
		while (true)
		{
			HeadValue oldHead = m_pHead.Get();
			if (m_pHead.TrySet(HeadValue{ nullptr, oldHead.tag + 1 }, oldHead))
			{
				m_count.Set(0);
				return oldHead.ptr;
			}
		}
	}


	// Implementation
protected:

	Head m_pHead;
	char m_Pad1[kCacheLineSize - sizeof(Head)];
	Atomic<int> m_count;
};

}
