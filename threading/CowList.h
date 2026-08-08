#pragma once

namespace SimpleLib
{

// CowList<T> - a copy-on-write vector for the specific UI-thread/audio-thread
// hand-off pattern used throughout Cantabile's execution graph. This is NOT a
// general purpose thread-safe container - it has exactly one intended usage
// pattern and relies on that pattern for its correctness:
//
//   - Exactly one "writer" thread (normally the UI thread) calls Add/InsertAt/
//     RemoveAt/Move/Remove to mutate the vector. These calls are NOT
//     thread-safe with respect to each other - they must all come from the
//     same thread, one at a time.
//
//   - Exactly one "reader" thread (normally the audio thread) calls
//     GetSnapshot() once per audio cycle to obtain the current, immutable
//     state of the vector as a CowListSnapshot<T>. GetSnapshot() must likewise
//     only ever be called from that one thread - it is not safe to call it
//     concurrently from two threads.
//
// GetSnapshot() returns a reference to an internally owned snapshot object.
// That reference is only valid until the NEXT call to GetSnapshot() (the
// previous snapshot is deleted at that point) - don't hold onto it across
// cycles.
//
// Each snapshot exposes three views:
//   - GetSize()/GetItem(i)          - the full current contents, in order.
//   - GetInsertedCount()/GetInsertedItem(i) - items added since the reader's
//                                     previous snapshot.
//   - GetDeletedCount()/GetDeletedItem(i)   - items removed since the
//                                     reader's previous snapshot.
//
// The inserted/deleted lists are a SET of items, not a log of operations:
// they tell you *which* objects were added or removed since the last
// snapshot, not the order, count, or interleaving of the Insert/Remove/Move
// calls that produced them, and not their position/index. In particular:
//   - If the same logical slot is inserted into and then removed from again
//     before the reader ever takes a snapshot, neither shows up at all (they
//     cancel out - see the reconciliation step in GetSnapshot()).
//   - Move() never appears in the inserted/deleted lists at all - a
//     reordering only shows up as a different item order in the full
//     GetItem() view.
// This is sufficient for identity-based bookkeeping (e.g. registering/
// revoking precedents for objects that came and went), but this class
// cannot be used to replay a positional edit history.
//
// Threading note: the writer reads fields directly off the previously
// "pending" snapshot object (to build the next merged one) while the reader
// may concurrently take that same object via GetSnapshot() and mutate it
// in-place during reconciliation. This is only race-free because of how
// StorePendingSnapshot's compare-exchange result is used: a successful CAS
// proves the reader could not yet have touched the object, and a failed CAS
// discards the writer's copy before its counts are ever consulted. That
// argument depends entirely on the strict single-writer/single-reader
// pattern above - it does not generalize to any other usage, so don't reuse
// this class outside that pattern without re-deriving the proof.
//
// Freeing note: GetSnapshot() runs on the audio thread and must not call
// into the heap allocator. So rather than deleting the outgoing m_current
// there, it's handed off to a single-slot "retired" pointer, which the
// writer thread frees off at the start of its next mutating call. A single
// slot (rather than a queue) is enough: a second retirement can only ever
// happen after the writer has produced another pending snapshot, and every
// mutating call reclaims the slot before doing anything else - so the slot
// is always empty again well before it could be needed a second time.
//
// T must be trivially copyable (this is enforced by a static_assert) -
// CowList is designed for small POD types / raw pointers, not owning
// objects. T must also be == comparable - it's used to identify items in
// Find/IndexOf/Remove and in the inserted/deleted reconciliation in
// GetSnapshot().

template <typename T>
class CowList;

template <typename T>
class CowListSnapshot
{
public:
	int GetSize() const
	{
		return size;
	}

	T GetItem(int index) const
	{
		assert(index >= 0 && index < size);
		return data[index];
	}

	int GetInsertedCount() const
	{
		return insertedCount;
	}

	T GetInsertedItem(int index) const
	{
		assert(index >= 0 && index < insertedCount);
		return data[size + index];
	}

	int GetDeletedCount() const
	{
		return deletedCount;
	}

	T GetDeletedItem(int index) const
	{
		assert(index >= 0 && index < deletedCount);
		return data[size + insertedRoom + index];
	}


private:
	CowListSnapshot(int size, int inserted, int deleted) :
		size(size),
		insertedCount(inserted),
		insertedRoom(inserted),
		deletedCount(deleted),
		deletedRoom(deleted)
	{
		data = (T*)malloc(sizeof(T) * (size + insertedRoom + deletedRoom));
	}

	~CowListSnapshot()
	{
		free(data);
	}

	T* GetItemBuffer()
	{
		return data;
	}

	T* GetInsertedBuffer()
	{
		return data + size;
	}

	T* GetDeletedBuffer()
	{
		return data + size + insertedRoom;
	}

	void ClearModifications()
	{
		insertedCount = 0;
		deletedCount = 0;
	}

	void TrimFromDeleted(int index)
	{
		assert(index >= 0 && index < deletedCount);

		T* delBuffer = GetDeletedBuffer();
		delBuffer[index] = delBuffer[deletedCount - 1];
		deletedCount--;
	}

	bool TrimFromInserted(T item)
	{
		T* insBuffer = GetInsertedBuffer();
		for (int i = 0; i < insertedCount; i++)
		{
			if (insBuffer[i] == item)
			{
				// Move the last inserted item into this slot
				insBuffer[i] = insBuffer[insertedCount - 1];
				insertedCount--;
				return true;
			}
		}
		return false;
	}

	int size;
	int insertedCount;
	int insertedRoom;
	int deletedCount;
	int deletedRoom;
	T* data;

	friend class CowList<T>;
};

template <typename T>
class CowList
{
	static_assert(std::is_trivially_copyable_v<T>,
		"T must be trivially copyable for use in CowList");

public:
	CowList(int initialSize = 16, int growSize = 16)
	{
		m_iInitialSize = initialSize;
		m_iGrowSize = growSize;
		m_pData = NULL;
		m_iSizeAllocated = 0;
		m_iSize = 0;
		m_pending.Set(nullptr);
		m_retired.Set(nullptr);
		m_current = new CowListSnapshot<T>(0, 0, 0);
	}

	~CowList()
	{
		Reset();
		delete m_current;
	}

	void Reset()
	{
		RemoveAll();
		free(m_pData);
		m_pData = NULL;
		m_iSizeAllocated = 0;
		m_iSize = 0;
		delete m_pending.Set(nullptr);
		delete m_retired.Set(nullptr);
		delete m_current;
		m_current = new CowListSnapshot<T>(0, 0, 0);
	}

	// Frees the snapshot (if any) that GetSnapshot() retired on the reader
	// thread since our last mutating call. Must run on the writer thread,
	// at the start of every mutating operation, before anything else.
	void ReclaimRetired()
	{
		delete m_retired.Set(nullptr);
	}

	void RemoveAll()
	{
		m_iSize = 0;
	}

	void GrowTo(int size)
	{
		if (size <= m_iSizeAllocated)
			return;

		m_pData = (T*)realloc(m_pData, sizeof(T) * size);
		m_iSizeAllocated = size;
	}

	int Add(const T& val)
	{
		return InsertAt(m_iSize, val);
	}

	int InsertAt(int iPosition, const T& val)
	{
		assert(iPosition >= 0);
		assert(iPosition <= GetSize());

		ReclaimRetired();

		// Grow?
		if (m_iSize + 1 >= m_iSizeAllocated)
		{
			GrowTo(m_iSize == 0 ? m_iInitialSize : m_iSizeAllocated + m_iGrowSize);
		}

		// Shuffle memory
		if (iPosition < m_iSize)
			memmove(m_pData + iPosition + 1, m_pData + iPosition, (m_iSize - iPosition) * sizeof(T));

		// Store value
		m_pData[iPosition] = val;

		// Update size
		m_iSize++;

		// Update snapshot
		CowListSnapshot<T>* pending = m_pending.Load();
		CowListSnapshot<T>* newSnapshot;
		if (pending)
		{
			newSnapshot = new CowListSnapshot<T>(m_iSize, pending->GetInsertedCount() + 1, pending->GetDeletedCount());
			memcpy(newSnapshot->GetItemBuffer(), m_pData, sizeof(T) * m_iSize);
			memcpy(newSnapshot->GetInsertedBuffer(), pending->GetInsertedBuffer(), sizeof(T) * pending->GetInsertedCount());
			memcpy(newSnapshot->GetDeletedBuffer(), pending->GetDeletedBuffer(), sizeof(T) * pending->GetDeletedCount());
			newSnapshot->GetInsertedBuffer()[pending->GetInsertedCount()] = val;
		}
		else
		{
			newSnapshot = new CowListSnapshot<T>(m_iSize, 1, 0);
			memcpy(newSnapshot->GetItemBuffer(), m_pData, sizeof(T) * m_iSize);
			newSnapshot->GetInsertedBuffer()[0] = val;
		}
		StorePendingSnapshot(newSnapshot, pending);

		return iPosition;
	}

	void RemoveAt(int iPosition)
	{
		assert(iPosition >= 0);
		assert(iPosition < GetSize());

		ReclaimRetired();

		// Capture deleted value
		T val = GetAt(iPosition);

		// Shuffle memory
		if (iPosition < GetSize() - 1)
			memmove(m_pData + iPosition, m_pData + iPosition + 1, (m_iSize - iPosition - 1) * sizeof(T));

		// Update size
		m_iSize--;

		// Update snapshot
		CowListSnapshot<T>* pending = m_pending.Load();
		CowListSnapshot<T>* newSnapshot;
		if (pending)
		{
			newSnapshot = new CowListSnapshot<T>(m_iSize, pending->GetInsertedCount(), pending->GetDeletedCount() + 1);
			memcpy(newSnapshot->GetItemBuffer(), m_pData, sizeof(T) * m_iSize);
			memcpy(newSnapshot->GetInsertedBuffer(), pending->GetInsertedBuffer(), sizeof(T) * pending->GetInsertedCount());
			memcpy(newSnapshot->GetDeletedBuffer(), pending->GetDeletedBuffer(), sizeof(T) * pending->GetDeletedCount());
			newSnapshot->GetDeletedBuffer()[pending->GetDeletedCount()] = val;
		}
		else
		{
			newSnapshot = new CowListSnapshot<T>(m_iSize, 0, 1);
			memcpy(newSnapshot->GetItemBuffer(), m_pData, sizeof(T) * m_iSize);
			newSnapshot->GetDeletedBuffer()[0] = val;
		}
		StorePendingSnapshot(newSnapshot, pending);
	}

	void Move(int iFrom, int iTo)
	{
		assert(iFrom >= 0 && iFrom < GetSize());
		assert(iTo >= 0 && iTo < GetSize());

		// Redundant?
		if (iFrom == iTo)
			return;

		ReclaimRetired();

		T temp = m_pData[iFrom];
		if (iTo < iFrom)
		{
			memmove(m_pData + iTo + 1, m_pData + iTo, (iFrom - iTo) * sizeof(T));
		}
		else
		{
			memmove(m_pData + iFrom, m_pData + iFrom + 1, (iTo - iFrom) * sizeof(T));
		}
		m_pData[iTo] = temp;

		// Update snapshot
		CowListSnapshot<T>* pending = m_pending.Load();
		CowListSnapshot<T>* newSnapshot;
		if (pending)
		{
			newSnapshot = new CowListSnapshot<T>(m_iSize, pending->GetInsertedCount(), pending->GetDeletedCount());
			memcpy(newSnapshot->GetItemBuffer(), m_pData, sizeof(T) * m_iSize);
			memcpy(newSnapshot->GetInsertedBuffer(), pending->GetInsertedBuffer(), sizeof(T) * pending->GetInsertedCount());
			memcpy(newSnapshot->GetDeletedBuffer(), pending->GetDeletedBuffer(), sizeof(T) * pending->GetDeletedCount());
		}
		else
		{
			// Create a new snap shot with one delete operation
			newSnapshot = new CowListSnapshot<T>(m_iSize, 0, 0);
			memcpy(newSnapshot->GetItemBuffer(), m_pData, sizeof(T) * m_iSize);
		}
		StorePendingSnapshot(newSnapshot, pending);
	}

	void StorePendingSnapshot(CowListSnapshot<T>* newSnapshot, CowListSnapshot<T>* oldSnapshot)
	{
		// Try to store it
		CowListSnapshot<T>* replaced = m_pending.CompareExchange(newSnapshot, oldSnapshot);

		if (replaced != oldSnapshot)
		{
			// The previous pending snapshot was replaced by another thread which now
			// has this list of modifications.  Clear the modifications and store again
			// which should always succeed.
			newSnapshot->ClearModifications();
			m_pending.Store(newSnapshot);
		}
		else
		{
			// The previous snap shot was never used, discard it
			delete replaced;
		}
	}

	CowListSnapshot<T>& GetSnapshot()
	{
		CowListSnapshot<T>* pending = m_pending.Set(nullptr);
		if (pending)
		{
			// Clean up items both added and removed
			if (pending->GetInsertedCount() > 0 && pending->GetDeletedCount() > 0)
			{
				for (int i = pending->GetDeletedCount() - 1; i >= 0; i--)
				{
					if (pending->TrimFromInserted(pending->GetDeletedItem(i)))
					{
						pending->TrimFromDeleted(i);
					}
				}
			}

			// Retire the current one and replace with the pending one. Must not
			// delete m_current here - this runs on the audio thread. Hand it
			// off for the writer thread to free instead (see ReclaimRetired).
			assert(m_retired.Get() == nullptr);
			m_retired.Set(m_current);
			m_current = pending;
		}
		else
		{
			m_current->ClearModifications();
		}

		return *m_current;
	}


	int Find(const T& arg)
	{
		for (int i = 0; i < GetSize(); i++)
		{
			if (GetAt(i) == arg)
				return i;
		}
		return -1;
	}

	void Remove(const T& arg)
	{
		RemoveAt(Find(arg));
	}

	int GetSize() const
	{
		return m_iSize;
	}

	T& operator[] (int index) const
	{
		return GetAt(index);
	}

	T& GetAt(int index) const
	{
		assert(index >= 0 && index < m_iSize);

		return m_pData[index];
	}

	int IndexOf(const T& item)
	{
		for (int i = 0; i < m_iSize; i++)
		{
			if (m_pData[i] == item)
				return i;
		}
		return -1;
	}

	/*
	typedef int(_cdecl* PFNCOMPARE)(T const& a, T const& b);
	typedef int(_cdecl* PFNCOMPAREQSORT)(const void* a, const void* b);

	void Sort(PFNCOMPARE pfnCompare)
	{
		qsort(m_pData, m_iSize, sizeof(T), (PFNCOMPAREQSORT)pfnCompare);
	}
	*/



protected:
	// Writer side mutable data
	T* m_pData;
	int m_iSizeAllocated;
	int m_iSize;
	int m_iGrowSize;
	int m_iInitialSize;

	// Pending immutable vector will be picked up on next snapshot
	Atomic<CowListSnapshot<T>*> m_pending;

	// Current snapshot of the vector used by reader side
	CowListSnapshot<T>* m_current;

	// Snapshot retired by the reader (GetSnapshot), awaiting deletion by the
	// writer thread - see ReclaimRetired() and the freeing note up top.
	Atomic<CowListSnapshot<T>*> m_retired;
};


}