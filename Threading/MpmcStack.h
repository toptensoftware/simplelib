#pragma once

#include "Atomic.h"

namespace SimpleLib
{

// Implements an intrusive multi-producer, multi-consumer unbound lifo stack
// Type `T` must have a member `T* next`
template <class T>
class alignas(kCacheLineSize) MpmcStack
{
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
		m_pHead.Set(nullptr);
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
			T* oldHead = m_pHead.Get();
			item->next = oldHead;
			bool isFirst = oldHead == nullptr;
			if (m_pHead.TrySet(item, oldHead))
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
			T* oldHead = m_pHead.Get();
			last->next = oldHead;
			bool isFirst = oldHead == nullptr;
			if (m_pHead.TrySet(first, oldHead))
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
			T* oldHead = m_pHead.Get();
			last->next = oldHead;
			bool isFirst = oldHead == nullptr;
			if (m_pHead.TrySet(first, oldHead))
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
			T* item = m_pHead.Get();

			// Empty list?
			if (item == nullptr)
				return item;

			// Update head
			if (m_pHead.TrySet(item->next, item))
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
			T* item = m_pHead.Get();
			if (m_pHead.TrySet(nullptr, item))
			{
				m_count.Set(0);
				return item;
			}
		}
	}


	// Implementation
protected:

	Atomic<T*> m_pHead;
	char m_Pad1[kCacheLineSize - sizeof(Atomic<T*>)];
	Atomic<int> m_count;
};

}