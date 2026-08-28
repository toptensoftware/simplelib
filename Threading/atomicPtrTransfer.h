#pragma once

#include <assert.h>
#include <type_traits>

#include "Atomic.h"

namespace SimpleLib
{

// Default disposal policy for AtomicPtrTransfer: plain `delete`.
template <typename T>
class TDelete
{
public:
	static void Release(T* value)
	{
		delete value;
	}
};


// A lock-free hand-off of object pointers between exactly two threads - a
// "sender" and a "receiver" - for the UI-thread / audio-thread pattern used
// throughout Cantabile.
//
// The sender (UI thread) builds a new object and Send()s it. The receiver
// (audio thread) picks it up on its next cycle with Receive() and can hold
// its previous object alongside the new one for as long as it likes - eg:
// to diff old against new - then hands the old one back with Return(). The
// sender collects handed-back objects with Reclaim() and frees or recycles
// them. The receiver never calls the allocator and never blocks.
//
//     UI / sender thread:                  Audio / receiver thread (per cycle):
//         T* n = new T(...);                   if (T* n = x.Receive())
//         if (T* old = x.Send(n))              {
//             recycle_or_delete(old);              diff(current, n);   // both in hand
//         while (T* r = x.Reclaim())               x.Return(current);
//             recycle_or_delete(r);                current = n;
//                                              }
//
// Two independent channels
// ------------------------
//   Forward (Send -> Receive): a single atomic slot, latest-wins. If the
//   sender publishes again before the receiver takes the previous object,
//   Send() hands that un-taken object straight back to the sender - the
//   receiver never saw it, so the sender simply re-owns it. The receiver
//   only ever sees the most recent object.
//
//   Return (Return -> Reclaim): an intrusive lock-free (Treiber) stack,
//   linked through T::next. Return() only ever pushes - it never blocks,
//   allocates, frees, or overwrites - so no matter how far the sender falls
//   behind, nothing handed back is ever lost. Reclaim() pulls the whole
//   stack over to the sender thread and doles it out one object per call.
//   Order is LIFO; use SpscQueue<T*> directly if you need FIFO.
//
// Requirements
// ------------
//   - T must have an accessible `T* next;` member. It is used as the stack
//     link only while an object sits in the return channel, and is dead
//     everywhere else - don't rely on its value once an object is back in
//     your hands.
//   - Exactly one sender thread and one receiver thread. Send()/Reclaim()
//     are sender-only; Receive()/Return() are receiver-only.
//   - Don't Send() a null pointer.
//   - Destroy the transfer only once both threads have stopped using it.
//
// Ownership
// ---------
//   - An object passed to Send() belongs to the transfer until the receiver
//     takes it with Receive(), or the sender gets it straight back from a
//     later Send().
//   - An object passed to Return() belongs to the transfer until the sender
//     collects it with Reclaim().
//   - Whatever is still in either channel at destruction is disposed of via
//     TRelease. The object the receiver is currently working on is never in
//     the transfer and stays the receiver's to keep.
//   - TRelease::Release() only ever runs on the sender thread (the Reclaim
//     path) or at destruction, never on the receiver thread, so it is free
//     to delete / allocate / lock.
template <typename T, typename TRelease = TDelete<T>>
class AtomicPtrTransfer
{
	static_assert(std::is_same_v<decltype(T::next), T*>,
		"T must have a 'T* next' member for the intrusive return stack");

public:
	AtomicPtrTransfer() = default;

	AtomicPtrTransfer(const AtomicPtrTransfer&) = delete;
	AtomicPtrTransfer& operator=(const AtomicPtrTransfer&) = delete;

	~AtomicPtrTransfer()
	{
		// Everything still parked in either channel belongs to the transfer.
		TRelease::Release(m_sent.Set(nullptr));
		ReleaseChain(m_reclaimList);
		ReleaseChain(m_returned.Set(nullptr));
	}

	// [sender] Publish `ptr` to the receiver. If the receiver never took the
	// previously sent object it is handed back here (and the sender re-owns
	// it); otherwise returns nullptr.
	T* Send(T* ptr)
	{
		assert(ptr != nullptr);
		return m_sent.Set(ptr);
	}

	// [sender] Collect one object the receiver has handed back, or nullptr
	// when none are pending. Call repeatedly to drain.
	T* Reclaim()
	{
		if (m_reclaimList == nullptr)
			m_reclaimList = m_returned.Set(nullptr);

		T* p = m_reclaimList;
		if (p != nullptr)
		{
			m_reclaimList = p->next;
			p->next = nullptr;
		}
		return p;
	}

	// [receiver] Take the most recently published object, or nullptr if
	// nothing new has been sent since the last call. Hands nothing back -
	// the receiver keeps whatever it was already holding.
	T* Receive()
	{
		return m_sent.Set(nullptr);
	}

	// [receiver] Hand an object back to the sender for disposal or reuse.
	// Never blocks or allocates. `ptr->next` is used as the stack link and
	// must not be touched by the caller until the object comes back from
	// Reclaim().
	void Return(T* ptr)
	{
		assert(ptr != nullptr);
		T* head = m_returned.Get();
		for (;;)
		{
			ptr->next = head;
			T* prev = m_returned.CompareExchange(ptr, head);
			if (prev == head)
				return;
			head = prev;
		}
	}

protected:
	static void ReleaseChain(T* p)
	{
		while (p != nullptr)
		{
			T* next = p->next;
			TRelease::Release(p);
			p = next;
		}
	}

	Atomic<T*> m_sent;			// forward channel: single slot, latest-wins
	Atomic<T*> m_returned;		// return channel: intrusive Treiber stack head (linked via T::next)
	T* m_reclaimList = nullptr;	// sender-thread-only: chain pulled from m_returned, not yet doled out
};



}
