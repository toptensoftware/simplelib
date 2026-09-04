#pragma once

#include "MpmcStack.h"

namespace SimpleLib
{

// MpmcLinkedQueue Class
// An intrusive, unbounded, multi-producer/multi-consumer FIFO queue.
// Type `T` must have a member `T* next`.
//
// Built on MpmcStack (LIFO): Write() pushes onto the stack, and ReadAll()
// pops the whole stack in one atomic step and reverses it into FIFO order.
// There is no single-item read - consumers drain a whole batch at a time.
template <class T>
class MpmcLinkedQueue
{
public:
	MpmcLinkedQueue() {}
	virtual ~MpmcLinkedQueue() {}

	void Reset()
	{
		m_stack.Reset();
	}

	int GetLikelyCount()
	{
		return m_stack.GetLikelyCount();
	}

	// Append an item to the queue.
	// Returns true if the queue transitioned from empty to non-empty
	// (useful for signalling a waiting consumer).
	bool Write(T* item)
	{
		return m_stack.Push(item);
	}

	// Atomically detach every queued item and return them as a chain in
	// FIFO order (oldest first), or nullptr if the queue was empty.
	// Only the final node's `next` is nullptr; a caller that wants to
	// re-Write() the nodes must clear each `next` as it walks the chain.
	T* ReadAll()
	{
		// Get everything from the stack (in LIFO order)
		T* list = m_stack.PopAll();
		if (list == nullptr)
			return nullptr;

		// Reverse it to FIFO order
		T* reversed = nullptr;
		while (list)
		{
			T* next = list->next;
			list->next = reversed;
			reversed = list;
			list = next;
		}

		// Done;
		return reversed;
	}


private:
	MpmcStack<T> m_stack;
};

}