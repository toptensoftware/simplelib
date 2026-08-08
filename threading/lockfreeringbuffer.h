#pragma once

#include "Atomic.h"

namespace SimpleLib
{


// LockFreeRingBuffer Class
// Stores a FIFO queue of items as a ring buffer
// Supports single reader/single writer thread only.
template <class T>
class alignas(kCacheLineSize) LockFreeRingBuffer
{
public:
// Construction
	LockFreeRingBuffer(int iSize)
	{
		m_iSize=iSize;
		m_pMem=(T*)malloc(sizeof(T) * iSize);
		m_pWrapPos=m_pMem+iSize;
		m_pWritePos.Set(m_pMem);
		m_pReadPos.Set(m_pMem);
	}

	virtual ~LockFreeRingBuffer()
	{
		free(m_pMem);
	}

	#define AdvancePtr(x) (((x)+1)==m_pWrapPos ? m_pMem : ((x)+1))

	void Reset(int iNewSize=-1)
	{
		if (iNewSize>=0 && iNewSize!=m_iSize)
		{
			free(m_pMem);
			m_pMem=(T*)malloc(sizeof(T) * iNewSize);
			m_iSize=iNewSize;
		}
		m_pWrapPos=m_pMem+m_iSize;
		m_pWritePos.Set(m_pMem);
		m_pReadPos.Set(m_pMem);
	}

	bool IsEmpty() const
	{
		T* readPos = m_pReadPos.Get();
		T* writePos = m_pWritePos.Get();
		return readPos == writePos;
	}

	bool IsFull() const
	{
		T* writePos = m_pWritePos.Get();
		T* readPos = m_pReadPos.Get();
		return AdvancePtr(writePos) == readPos;
	}

	bool Peek(T& Value)
	{
		if (IsEmpty())
			return false;

		T* readPos = m_pReadPos.Load();
		Value = *readPos;

		return true;
	}

	bool Peek(int offset, T& Value)
	{
		// In range?
		if (offset < 0 || offset >= GetSize())
			return false;

		// Calculate wrapped position of this item
		T* readPos = m_pReadPos.Load();
		T* pPos = readPos + offset;
		if (pPos >= m_pWrapPos)
		{
			pPos -= m_iSize;
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
		if (IsEmpty())
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
	void MustWrite(const T& t)
	{
		if (IsFull())
		{
			assert(false);
			return;
		}

		// Copy it
		T* writePos = m_pWritePos.Get();
		Constructor(writePos, t);

		// Store next write pos
		m_pWritePos.Set(AdvancePtr(writePos));
	}

	// Write to end
	bool TryWrite(const T& t)
	{
		if (IsFull())
		{
			return false;
		}

		// Copy it
		T* writePos = m_pWritePos.Load();
		Constructor(writePos, t);

		// Store next write pos
		m_pWritePos.Store(AdvancePtr(writePos));

		return true;
	}

	int GetCapacity()
	{
		return m_iSize;
	}

	int GetCount()
	{
		T* writePos = m_pWritePos.Get();
		T* readPos = m_pReadPos.Get();
		int iSize = int(writePos - readPos);
		if (iSize < 0)
			iSize += m_iSize;
		return iSize;
	}

	T& GetAt(int index)
	{
		assert(index >= 0 && index < GetCount());

		// Find the Nth item
		T* readPos = m_pReadPos.Get();
		T* p = const_cast<T*>(readPos + index);

		// Check for wrap around
		if (p >= m_pWrapPos)
			p -= m_iSize;

		return *p;
	}



	// Implementation
protected:
	// Attributes
	int m_iSize;
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