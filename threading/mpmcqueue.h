#pragma once

#include "Atomic.h"

namespace SimpleLib
{

// MpmcQueue Class
// Stores a FIFO queue of items as a bounded ring buffer.
// Supports multiple concurrent reader and writer threads (MPMC).
// Based on Dmitry Vyukov's bounded MPMC queue algorithm:
//		https://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
template <class T>
class alignas(kCacheLineSize) MpmcQueue
{
public:
	// Construction
	MpmcQueue(int iSize)
	{
		m_pCells = nullptr;
		Reset(iSize);
	}

	virtual ~MpmcQueue()
	{
		if (m_pCells)
			free(m_pCells);
	}

	void Reset(int iSize)
	{
		// iSize == 1 is degenerate: the sequence number a Read() sets to
		// free a cell for its next lap collides with the number that same
		// cell had immediately after being written, so a second concurrent
		// writer can't tell "free" from "written but not yet read" and
		// silently clobbers unread data instead of reporting the queue full.
		assert(iSize != 1);
		assert((iSize & (iSize - 1)) == 0);		// Must be a power of two

		if (m_pCells)
			free(m_pCells);

		m_iSize = iSize;
		m_iMask = (uint32_t)iSize - 1;
		m_pCells = (Cell*)malloc(sizeof(Cell) * iSize);
		for (int i = 0; i < iSize; i++)
			m_pCells[i].m_iSequence.Set(i);

		// Cell sequence numbers above assume enqueue/dequeue start at position
		// 0 - must reset these too, or a Reset() on an already-used queue
		// leaves them stale and inconsistent with the fresh cells
		m_iEnqueuePos.Set(0);
		m_iDequeuePos.Set(0);
	}

	bool IsEmpty()
	{
		return GetCount() == 0;
	}

	bool IsFull()
	{
		return GetCount() >= m_iSize;
	}

	// Write to end
	bool TryWrite(const T& t)
	{
		Cell* pCell;
		uint32_t iPos = m_iEnqueuePos.Get();
		for (;;)
		{
			pCell = &m_pCells[iPos & m_iMask];
			uint32_t iSeq = pCell->m_iSequence.Get();
			int32_t iDif = (int32_t)(iSeq - iPos);
			if (iDif == 0)
			{
				// Cell is free for this lap - try to claim it
				if (m_iEnqueuePos.TrySet(iPos + 1, iPos) == iPos)
					break;
			}
			else if (iDif < 0)
			{
				// Queue is full
				return false;
			}
			else
			{
				// Lost the race for this slot - re-read and retry
				iPos = m_iEnqueuePos.Get();
			}
		}

		// Store the value and mark the cell as readable
		Constructor(&pCell->m_Value, t);
		pCell->m_iSequence.Set(iPos + 1);

		return true;
	}

	// Write to end
	void MustWrite(const T& t)
	{
		if (!TryWrite(t))
		{
			assert(false);
		}
	}

	// Read and remove the oldest written value
	bool Read(T& Value)
	{
		Cell* pCell;
		uint32_t iPos = m_iDequeuePos.Get();
		for (;;)
		{
			pCell = &m_pCells[iPos & m_iMask];
			uint32_t iSeq = pCell->m_iSequence.Get();
			int32_t iDif = (int32_t)(iSeq - (iPos + 1));
			if (iDif == 0)
			{
				// Cell has been written - try to claim it
				if (m_iDequeuePos.TrySet(iPos + 1, iPos) == iPos)
					break;
			}
			else if (iDif < 0)
			{
				// Queue is empty
				return false;
			}
			else
			{
				// Lost the race for this slot - re-read and retry
				iPos = m_iDequeuePos.Get();
			}
		}

		// Copy out the value and mark the cell free for the next lap
		Destructor(&Value);
		memcpy(&Value, const_cast<T*>(&pCell->m_Value), sizeof(T));
		pCell->m_iSequence.Set(iPos + (uint32_t)m_iSize);

		return true;
	}

	int GetCapacity()
	{
		return m_iSize;
	}

	// Approximate only - since other threads may be concurrently
	// reading/writing, the true count may have already changed by
	// the time this returns.
	int GetCount()
	{
		int32_t iCount = (int32_t)(m_iEnqueuePos.Get() - m_iDequeuePos.Get());
		if (iCount < 0)
			iCount = 0;
		return iCount;
	}

	// Implementation
protected:
	struct Cell
	{
		Atomic<uint32_t> m_iSequence;
		T m_Value;
	};

	// Attributes
	int m_iSize = 0;
	uint32_t m_iMask = 0;
	Cell* m_pCells = nullptr;

	// m_iEnqueuePos and m_iDequeuePos are CAS targets hit independently by
	// every producer and every consumer thread respectively - pad them
	// onto separate cache lines so contention on one doesn't cause false
	// sharing with the other
	char m_Pad0[kCacheLineSize];
	Atomic<uint32_t> m_iEnqueuePos;
	char m_Pad1[kCacheLineSize - sizeof(Atomic<uint32_t>)];
	Atomic<uint32_t> m_iDequeuePos;
	char m_Pad2[kCacheLineSize - sizeof(Atomic<uint32_t>)];

	// Operations
};

}