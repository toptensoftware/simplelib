#pragma once

#include <assert.h>

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
// to diff old against new - then hands the old one back with Return(). By
// default Send() disposes handed-back objects (via TRelease) for the sender;
// use SendNoReclaim() + Reclaim() to collect and recycle them yourself. The
// receiver never calls the allocator and never blocks.
//
//     UI / sender thread:                  Audio / receiver thread (per cycle):
//         x.Send(new T(...));                  if (T* n = x.Receive())
//                                              {
//                                                  if (current)
//                                                  {
//                                                      diff(current, n);   // both in hand
//                                                      x.Return(current);
//                                                  }
//                                                  current = n;
//                                              }
//
//   Or, to recycle rather than delete, on the sender thread:
//         T* n = get_from_pool();
//         if (T* old = x.SendNoReclaim(n))
//             pool_return(old);
//         while (T* r = x.Reclaim())
//             pool_return(r);
//
// Non-intrusive
// -------------
//   T has no requirements at all - the transfer boxes each pointer in a
//   small internally-allocated Node for its trip through the channels. Nodes
//   are allocated and freed only on the sender thread (SendNoReclaim /
//   Reclaim, plus the destructor) and recycled through a sender-local free
//   list, so a warm transfer does no allocation at all. The receiver still
//   never touches the allocator: every object reaches it riding a Node, and
//   Receive() keeps that Node so Return() can send the same one back down
//   the return channel.
//
// Two independent channels
// ------------------------
//   Forward (Send -> Receive): a single atomic slot, latest-wins. If the
//   sender publishes again before the receiver takes the previous object,
//   that un-taken object comes straight back to the sender - the receiver
//   never saw it. Send() disposes it via TRelease; SendNoReclaim() returns
//   it. The receiver only ever sees the most recent object.
//
//   Return (Return -> Reclaim): a lock-free (Treiber) stack of Nodes.
//   Return() only ever pushes - it never blocks, allocates, frees, or
//   overwrites - so no matter how far the sender falls behind, nothing
//   handed back is ever lost. Reclaim() pulls the whole stack over to the
//   sender thread and doles it out one object per call. Order is LIFO; use
//   SpscQueue<T*> directly if you need FIFO.
//
// Requirements
// ------------
//   - Exactly one sender thread and one receiver thread. Send()/Reclaim()
//     are sender-only; Receive()/Return() are receiver-only.
//   - Don't Send() a null pointer.
//   - Only Return() an object this receiver took with Receive() and still
//     holds - Return() asserts otherwise.
//   - Destroy the transfer only once both threads have stopped using it.
//
// Ownership
// ---------
//   - An object passed to Send()/SendNoReclaim() belongs to the transfer
//     until the receiver takes it with Receive(), or a later send bounces it
//     back (SendNoReclaim returns it; Send disposes it via TRelease).
//   - An object passed to Return() belongs to the transfer until the sender
//     collects it with Reclaim().
//   - Whatever is still in either channel at destruction is disposed of via
//     TRelease. Objects the receiver took with Receive() and has not yet
//     Return()ed are never in the transfer and stay the receiver's to keep.
//   - TRelease::Release() only ever runs on the sender thread (the Reclaim
//     path) or at destruction, never on the receiver thread, so it is free
//     to delete / allocate / lock.
template <typename T, typename TRelease = TDelete<T>>
class AtomicPtrTransfer
{
	// Box carrying one user pointer through the channels. `next` links the
	// Node into whichever list currently owns it - the sender free list, the
	// receiver's held list, or the return Treiber stack - never more than
	// one at a time, and each has a single owning thread while it does.
	struct Node
	{
		T* ptr;
		Node* next;
	};

public:
	AtomicPtrTransfer() = default;

	AtomicPtrTransfer(const AtomicPtrTransfer&) = delete;
	AtomicPtrTransfer& operator=(const AtomicPtrTransfer&) = delete;

	~AtomicPtrTransfer()
	{
		// Forward slot, return stack and the not-yet-doled reclaim list all
		// hold objects the transfer owns: dispose via TRelease, free the Node.
		if (Node* node = m_sent.Set(nullptr))
		{
			TRelease::Release(node->ptr);
			delete node;
		}
		ReleaseNodes(m_reclaimList);
		ReleaseNodes(m_returned.Set(nullptr));

		// The receiver keeps the objects it was still holding; free the Nodes.
		DeleteNodes(m_heldList);
		DeleteNodes(m_freeList);
	}

	// [sender] Publish `ptr` to the receiver and dispose - via TRelease - of
	// everything the transfer hands back in the process: every object the
	// receiver has returned since the last send, plus the previously sent
	// object if the receiver never took it. After Send() the sender owns
	// nothing and has no cleanup of its own to do. Use SendNoReclaim() to
	// recycle those objects yourself instead.
	void Send(T* ptr)
	{
		while (T* r = Reclaim())
			TRelease::Release(r);
		if (T* old = SendNoReclaim(ptr))
			TRelease::Release(old);
	}

	// [sender] Publish `ptr` to the receiver without touching the return
	// channel. If the receiver never took the previously sent object it is
	// handed back here (and the sender re-owns it); otherwise returns nullptr.
	// The sender is responsible for draining Reclaim() itself.
	T* SendNoReclaim(T* ptr)
	{
		assert(ptr != nullptr);

		Node* node = AllocNode();
		node->ptr = ptr;
		node->next = nullptr;

		Node* bounced = m_sent.Set(node);
		if (bounced == nullptr)
			return nullptr;

		T* old = bounced->ptr;
		FreeNode(bounced);
		return old;
	}

	// [sender] Collect one object the receiver has handed back, or nullptr
	// when none are pending. Call repeatedly to drain.
	T* Reclaim()
	{
		if (m_reclaimList == nullptr)
			m_reclaimList = m_returned.Set(nullptr);

		Node* node = m_reclaimList;
		if (node == nullptr)
			return nullptr;

		m_reclaimList = node->next;
		T* ptr = node->ptr;
		FreeNode(node);
		return ptr;
	}

	// [receiver] Take the most recently published object, or nullptr if
	// nothing new has been sent since the last call. Hands nothing back -
	// the receiver keeps whatever it was already holding.
	T* Receive()
	{
		Node* node = m_sent.Set(nullptr);
		if (node == nullptr)
			return nullptr;

		// Keep the Node - Return() sends this same one back down the return
		// channel, so the receiver never has to allocate one.
		node->next = m_heldList;
		m_heldList = node;
		return node->ptr;
	}

	// [receiver] Hand an object back to the sender for disposal or reuse.
	// Never blocks or allocates. `ptr` must be an object this receiver took
	// with Receive() and still holds.
	void Return(T* ptr)
	{
		assert(ptr != nullptr);

		// Unlink the Node handed out with `ptr` from the receiver-local held
		// list (Node::next is receiver-owned while the Node sits here).
		Node** link = &m_heldList;
		while (*link != nullptr && (*link)->ptr != ptr)
			link = &(*link)->next;

		assert(*link != nullptr && "Return() of an object this receiver isn't holding");
		Node* node = *link;
		if (node == nullptr)
			return;
		*link = node->next;

		// Push it onto the return Treiber stack.
		Node* head = m_returned.Get();
		for (;;)
		{
			node->next = head;
			Node* prev = m_returned.CompareExchange(node, head);
			if (prev == head)
				return;
			head = prev;
		}
	}

protected:
	// [sender] Node lifetime: recycle through a sender-local free list,
	// falling back to the allocator. Both are sender-thread-only.
	Node* AllocNode()
	{
		Node* node = m_freeList;
		if (node == nullptr)
			return new Node;
		m_freeList = node->next;
		return node;
	}

	void FreeNode(Node* node)
	{
		node->next = m_freeList;
		m_freeList = node;
	}

	static void ReleaseNodes(Node* node)
	{
		while (node != nullptr)
		{
			Node* next = node->next;
			TRelease::Release(node->ptr);
			delete node;
			node = next;
		}
	}

	static void DeleteNodes(Node* node)
	{
		while (node != nullptr)
		{
			Node* next = node->next;
			delete node;
			node = next;
		}
	}

	Atomic<Node*> m_sent;			// forward channel: single slot, latest-wins
	Atomic<Node*> m_returned;		// return channel: Treiber stack head (linked via Node::next)
	Node* m_reclaimList = nullptr;	// sender-only: chain pulled from m_returned, not yet doled out
	Node* m_freeList = nullptr;		// sender-only: recycled Nodes
	Node* m_heldList = nullptr;		// receiver-only: Nodes for objects the receiver is holding
};



}
