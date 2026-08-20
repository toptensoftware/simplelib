#pragma once

#include "Atomic.h"

namespace SimpleLib
{


// SpscQueue Class
// Stores a FIFO queue of items as a ring buffer
// Supports single reader/single writer thread only.
template <class T>
class alignas(kCacheLineSize) SpscQueue
{
public:
// Construction
	SpscQueue(int iCapacity)
	{
		m_iCapacity=iCapacity;
		m_pMem=(T*)malloc(sizeof(T) * iCapacity);
		m_pWrapPos=m_pMem+iCapacity;
		m_pWritePos.Set(m_pMem);
		m_pReadPos.Set(m_pMem);
	}

	virtual ~SpscQueue()
	{
		RemoveAll();
		free(m_pMem);
	}

	#define AdvancePtr(x) (((x)+1)==m_pWrapPos ? m_pMem : ((x)+1))

	void Reset(int iNewCapacity=-1)
	{
		// Destroy any elements still in the queue before discarding them -
		// their storage is either about to be freed (capacity change) or
		// silently overwritten by future writes (same capacity), and neither
		// path would otherwise ever run their destructors.
		RemoveAll();

		if (iNewCapacity>=0 && iNewCapacity!=m_iCapacity)
		{
			free(m_pMem);
			m_pMem=(T*)malloc(sizeof(T) * iNewCapacity);
			m_iCapacity=iNewCapacity;
		}
		m_pWrapPos=m_pMem+m_iCapacity;
		m_pWritePos.Set(m_pMem);
		m_pReadPos.Set(m_pMem);
	}

	bool IsLikelyEmpty() const
	{
		T* readPos = m_pReadPos.Get();
		T* writePos = m_pWritePos.Get();
		return readPos == writePos;
	}

	bool IsLikelyFull() const
	{
		T* writePos = m_pWritePos.Get();
		T* readPos = m_pReadPos.Get();
		return AdvancePtr(writePos) == readPos;
	}

	bool Peek(T& Value)
	{
		if (IsLikelyEmpty())
			return false;

		T* readPos = m_pReadPos.Get();
		Value = *readPos;

		return true;
	}

	bool Peek(int offset, T& Value)
	{
		// In range?
		if (offset < 0 || offset >= GetLikelyCount())
			return false;

		// Calculate wrapped position of this item
		T* readPos = m_pReadPos.Get();
		T* pPos = readPos + offset;
		if (pPos >= m_pWrapPos)
		{
			pPos -= m_iCapacity;
		}

		// Return it
		Value = *pPos;
		return true;
	}

	void RemoveAll()
	{
		T temp;
		while (Read(temp))
		{
		}
	}


	// Read and remove the oldest written value
	bool Read(T& Value)
	{
		// Quit if empty
		if (IsLikelyEmpty())
		{
			return false;
		}

		// Copy value
		T* readPos = m_pReadPos.Get();
		Destructor(&Value);
		memcpy(&Value, const_cast<T*>(readPos), sizeof(T));

		// Advance read position
		m_pReadPos.Set(AdvancePtr(readPos));

		return true;
	}

	// Write to end
	// Returns true if the queue transitioned from empty to non-empty
	bool MustWrite(const T& t)
	{
		if (IsLikelyFull())
		{
			assert(false);
			return false;
		}

		bool wasEmpty = IsLikelyEmpty();

		// Copy it
		T* writePos = m_pWritePos.Get();
		Constructor(writePos, t);

		// Store next write pos
		m_pWritePos.Set(AdvancePtr(writePos));

		return wasEmpty;
	}

	// Write to end
	bool TryWrite(const T& t)
	{
		if (IsLikelyFull())
		{
			return false;
		}

		// Copy it
		T* writePos = m_pWritePos.Get();
		Constructor(writePos, t);

		// Store next write pos
		m_pWritePos.Set(AdvancePtr(writePos));

		return true;
	}

	int GetCapacity()
	{
		return m_iCapacity;
	}

	int GetLikelyCount()
	{
		T* writePos = m_pWritePos.Get();
		T* readPos = m_pReadPos.Get();
		int iCount = int(writePos - readPos);
		if (iCount < 0)
			iCount += m_iCapacity;
		return iCount;
	}

	T& GetLikelyAt(int index)
	{
		assert(index >= 0 && index < GetLikelyCount());

		// Find the Nth item
		T* readPos = m_pReadPos.Get();
		T* p = const_cast<T*>(readPos + index);

		// Check for wrap around
		if (p >= m_pWrapPos)
			p -= m_iCapacity;

		return *p;
	}



	// Implementation
protected:
	// Attributes
	int m_iCapacity;
	T* m_pMem;
	T* m_pWrapPos;

	// m_pWritePos and m_pReadPos are each written by a different thread
	// (writer and reader respectively) - pad them onto separate cache
	// lines so writes to one don't invalidate the other's line
	char m_Pad0[kCacheLineSize];
	Atomic<T*> m_pWritePos;
	char m_Pad1[kCacheLineSize - sizeof(Atomic<T*>)];
	Atomic<T*> m_pReadPos;
	char m_Pad2[kCacheLineSize - sizeof(Atomic<T*>)];

	// Operations
};

}