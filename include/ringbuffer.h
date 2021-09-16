#ifndef __simplelib_ringbuffer_h__
#define __simplelib_ringbuffer_h__

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "semantics.h"


namespace SimpleLib
{
	// Implements a simple ring buffer.

	// If only the (Try)Enqueue and (Try)Dequeue methods are used then this class is safe
	// for single reader / single writer multi threaded use
	template <typename T, typename TSem = SValue>
	class CRingBuffer
	{
	public:
		// Constructor
		CRingBuffer(int iCapacity)
		{
			m_iCapacityPlusOne = iCapacity + 1;
			m_pMem = (T*)malloc(sizeof(T) * m_iCapacityPlusOne);
			m_pWritePos = m_pMem;
			m_pReadPos = m_pMem;
			m_bOverflow = false;
		}

		// Destructor
		virtual ~CRingBuffer()
		{
			RemoveAll();
			free(m_pMem);
		}

		// Types
		typedef CRingBuffer<T, TSem> _CRingBuffer;

		// Reset and optionally resize 
		void Reset(int iNewCapacity = 0)
		{
			RemoveAll();
			if (iNewCapacity && iNewCapacity + 1 != m_iCapacityPlusOne)
			{
				free(m_pMem);

				m_iCapacityPlusOne = iNewCapacity + 1;
				m_pMem = (T*)malloc(sizeof(T) * m_iCapacityPlusOne);
				m_pWritePos = m_pMem;
				m_pReadPos = m_pMem;
				m_bOverflow = false;
			}
		}

		// Check if buffer is empty
		bool IsEmpty() const
		{
			return m_pReadPos == m_pWritePos;
		}

		// Check if buffer is full
		bool IsFull() const
		{
			return AdvancePtr(m_pWritePos) == m_pReadPos;
		}

		// Check if buffer has overflown
		bool IsOverflow() const
		{
			return m_bOverflow;
		}

		// Try to add an item to the end of the queue
		bool TryEnqueue(const T& t)
		{
			// Check if full
			if (IsFull())
			{
				m_bOverflow = true;
				return false;
			}

			T* pNextWritePos = AdvancePtr(m_pWritePos);

			Constructor(m_pWritePos, TSem::OnAdd(t, this));

			// Store next write pos
			m_pWritePos = pNextWritePos;

			return true;
		}

		// Add an item to the end of the queue (assert if can't)
		void Enqueue(const T& t)
		{
			// Check if full
			assert(!IsFull());

			T* pNextWritePos = AdvancePtr(m_pWritePos);

			Constructor(m_pWritePos, TSem::OnAdd(t, this));

			// Store next write pos
			m_pWritePos = pNextWritePos;
		}

		// Add an item to the end of the queue (assert if can't)
		void GrowEnqueue(const T& t)
		{
			if (IsFull())
			{
				// Store old memory pointer
				T* pOldMem = m_pMem;

				// Grow the buffer and adjust pointers incase memory was moved
				int newCapPlusOne = (m_iCapacityPlusOne - 1) * 2 + 1;
				m_pMem = (T*)realloc(m_pMem, newCapPlusOne * sizeof(T));
				m_pReadPos = m_pMem + (m_pReadPos - pOldMem);
				m_pWritePos = m_pMem + (m_pWritePos - pOldMem);

				if (m_pReadPos > m_pWritePos)
				{
					// Move elements
					T* pOldEndPos = m_pMem + m_iCapacityPlusOne;
					if (pOldEndPos > m_pReadPos)
						memmove(m_pReadPos + (newCapPlusOne - m_iCapacityPlusOne), m_pReadPos, (pOldEndPos - m_pReadPos) * sizeof(T));

					// Shuffle the write pointer
					m_pReadPos += newCapPlusOne - m_iCapacityPlusOne;
				}

				// Update capacity
				m_iCapacityPlusOne = newCapPlusOne;
			}

			assert(!IsFull());

			T* pNextWritePos = AdvancePtr(m_pWritePos);

			Constructor(m_pWritePos, TSem::OnAdd(t, this));

			// Store next write pos
			m_pWritePos = pNextWritePos;
		}

		// Try to remove an item from the head of the queue
		bool TryDequeue(T& t)
		{
			if (IsEmpty())
				return false;

			t = Dequeue();
			return true;
		}

		// Remove an item from the head of the queue (assert if empty)
		T Dequeue()
		{
			assert(!IsEmpty());

			// Remember where we are
			T* pSave = m_pReadPos;

			// Advance read position
			m_pReadPos = AdvancePtr(m_pReadPos);

			// Return data
			T temp = *pSave;

			// Detach
			TSem::OnDetach(*pSave, this);

			// and destroy
			Destructor(pSave);

			// Done
			return temp;
		}

		// Try to get the item at the head of the queue, but don't remove it
		bool TryPeek(T& t)
		{
			if (IsEmpty())
				return false;

			t = Peek();
			return true;
		}

		// Get the item from the head of the queue but don't remove it
		T Peek()
		{
			assert(!IsEmpty());

			// Remember where we are
			return *m_pReadPos;
		}

		// Try to add an item to the head of the queue
		bool TryUnenqueue(T& t)
		{
			if (IsEmpty())
				return false;

			t = Unenqueue();
			return true;
		}

		// Add an item to the head of the queue
		T Unenqueue()
		{
			assert(!IsEmpty());
			m_pWritePos = RewindPtr(m_pWritePos);

			// Return data
			T temp = *m_pWritePos;

			// Detach
			TSem::OnDetach(*m_pWritePos, this);

			// and destroy
			Destructor(m_pWritePos);

			// Done
			return temp;
		}

		// Try to peek at the last item in the buffer
		bool TryPeekLast(T& t)
		{
			if (IsEmpty())
				return false;

			t = PeekLast();
			return true;
		}

		// Peek at the last item in the buffer (assert if empty)
		T PeekLast()
		{
			assert(!IsEmpty());
			return GetAt(GetCount() - 1);
		}

		// Remove all items from the buffer
		void RemoveAll()
		{
			while (!IsEmpty())
			{
				Dequeue();
			}

			m_bOverflow = false;
		}

		// Get the buffer's capacity
		int GetCapacity() const
		{
			return m_iCapacityPlusOne - 1;
		}

		// Get the number of items in the collection
		int GetCount() const
		{
			if (m_pWritePos >= m_pReadPos)
				return m_pWritePos - m_pReadPos;
			else
				return m_iCapacityPlusOne - (m_pReadPos - m_pWritePos);
		}

		// Get the item at a specified position in the collection
		T GetAt(int iPos) const
		{
			assert(iPos >= 0 && iPos < GetCount());

			// Next position
			T* p = m_pReadPos + iPos;

			// Past end?
			if (p >= m_pMem + m_iCapacityPlusOne)
			{
				// Back to start
				p -= m_iCapacityPlusOne;
			}

			return *p;
		}

		// [] operator
		T operator [] (int iPos) const
		{
			return GetAt(iPos);
		}



		DEPRECATED("Use TryDequeue() instead")
			bool Dequeue(T& t) { return TryDequeue(t); }
		DEPRECATED("Use TryPeek() instead")
			bool Peek(T& t) { return TryPeek(t); }
		DEPRECATED("Use TryUnqueue() instead")
			bool Unenqueue(T& t) { return TryUnenqueue(t); }
		DEPRECATED("Use TryPeekLast() instead")
			bool PeekLast(T& t) { return TryPeekLast(t); }
		DEPRECATED("Use GetCount() instead")
			int GetSize() const { return GetCount(); }

		// Implementation
	protected:
		// Attributes
		T* m_pMem;
		int		m_iCapacityPlusOne;
		T* m_pWritePos;
		T* m_pReadPos;
		bool	m_bOverflow;

		// Advance pointer to next position
		T* AdvancePtr(T* p)	const
		{
			// Next position
			p++;

			// At end?
			if (p == m_pMem + m_iCapacityPlusOne)
			{
				// Back to start
				p = m_pMem;
			}

			return p;
		}

		// Rewind pointer to previous position
		T* RewindPtr(T* p)	const
		{
			// At start?
			if (p == m_pMem)
			{
				p = m_pMem + m_iCapacityPlusOne - 1;
			}
			else
			{
				p--;
			}

			return p;
		}

	private:
		// Unsupported
		CRingBuffer(const CRingBuffer& Other);
		CRingBuffer& operator=(const CRingBuffer& Other);
	};


} // namespace

#endif  // __simplelib_ringbuffer_h__