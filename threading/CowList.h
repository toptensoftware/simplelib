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
//   - GetCount()/GetItem(i)         - the full current contents, in order.
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
	int GetCount() const
	{
		return count;
	}

	T GetItem(int index) const
	{
		assert(index >= 0 && index < count);
		return data[index];
	}

	int GetInsertedCount() const
	{
		return insertedCount;
	}

	T GetInsertedItem(int index) const
	{
		assert(index >= 0 && index < insertedCount);
		return data[count + index];
	}

	int GetDeletedCount() const
	{
		return deletedCount;
	}

	T GetDeletedItem(int index) const
	{
		assert(index >= 0 && index < deletedCount);
		return data[count + insertedRoom + index];
	}


private:
	CowListSnapshot(int count, int inserted, int deleted) :
		count(count),
		insertedCount(inserted),
		insertedRoom(inserted),
		deletedCount(deleted),
		deletedRoom(deleted)
	{
		data = (T*)malloc(sizeof(T) * (count + insertedRoom + deletedRoom));
	}

	~CowListSnapshot()
	{
		free(data);
	}

	// Intrusive link for the writer-reclaimed retirement stack (see
	// CowList::m_retired) - GetSnapshot()'s own retire step isn't atomic
	// (there's reconciliation work between consuming `pending` and actually
	// retiring m_current), so an arbitrary number of writer mutating calls
	// can complete in that window before the writer gets a chance to
	// reclaim. m_retired must therefore be able to hold more than one
	// outstanding retirement, not just the most recent.
	CowListSnapshot<T>* next = nullptr;

	T* GetItemBuffer()
	{
		return data;
	}

	T* GetInsertedBuffer()
	{
		return data + count;
	}

	T* GetDeletedBuffer()
	{
		return data + count + insertedRoom;
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

	int count;
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
	CowList(int initialCapacity = 16, int growCapacity = 16)
	{
		m_iInitialCapacity = initialCapacity;
		m_iGrowCapacity = growCapacity;
		m_pData = NULL;
		m_iCapacity = 0;
		m_iCount = 0;
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
		m_iCapacity = 0;
		m_iCount = 0;
		delete m_pending.Set(nullptr);
		ReclaimRetired();
		delete m_current;
		m_current = new CowListSnapshot<T>(0, 0, 0);
	}

	// Frees whatever snapshot(s) GetSnapshot() retired on the reader thread
	// since our last mutating call. Must run on the writer thread, at the
	// start of every mutating operation, before anything else.
	//
	// m_retired is a lock-free stack (Treiber stack), not a single slot:
	// GetSnapshot()'s retire step isn't atomic (there's reconciliation work
	// between consuming `pending` and actually pushing the retired
	// m_current), so an arbitrary number of writer mutating calls can run to
	// completion in that window, each finding nothing here yet. Whichever
	// writer call runs after the reader's retirement(s) finally land must be
	// able to reclaim all of them, not just the most recent one.
	void ReclaimRetired()
	{
		CowListSnapshot<T>* node = m_retired.Set(nullptr);
		while (node)
		{
			CowListSnapshot<T>* next = node->next;
			delete node;
			node = next;
		}
	}

	void RemoveAll()
	{
		m_iCount = 0;
	}

	// Ensure allocated capacity is at least `capacity` (does not shrink)
	void SetCapacity(int capacity)
	{
		if (capacity <= m_iCapacity)
			return;

		m_pData = (T*)realloc(m_pData, sizeof(T) * capacity);
		m_iCapacity = capacity;
	}

	int Add(const T& val)
	{
		return InsertAt(m_iCount, val);
	}

	int InsertAt(int iPosition, const T& val)
	{
		assert(iPosition >= 0);
		assert(iPosition <= GetCount());

		ReclaimRetired();

		// Grow?
		if (m_iCount + 1 >= m_iCapacity)
		{
			SetCapacity(m_iCount == 0 ? m_iInitialCapacity : m_iCapacity + m_iGrowCapacity);
		}

		// Shuffle memory
		if (iPosition < m_iCount)
			memmove(m_pData + iPosition + 1, m_pData + iPosition, (m_iCount - iPosition) * sizeof(T));

		// Store value
		m_pData[iPosition] = val;

		// Update count
		m_iCount++;

		// Update snapshot
		CowListSnapshot<T>* pending = m_pending.Get();
		CowListSnapshot<T>* newSnapshot = BuildSnapshot(pending, ChangeKind::Inserted, &val);
		StorePendingSnapshot(newSnapshot, pending, ChangeKind::Inserted, &val);

		return iPosition;
	}

	void RemoveAt(int iPosition)
	{
		assert(iPosition >= 0);
		assert(iPosition < GetCount());

		ReclaimRetired();

		// Capture deleted value
		T val = GetAt(iPosition);

		// Shuffle memory
		if (iPosition < GetCount() - 1)
			memmove(m_pData + iPosition, m_pData + iPosition + 1, (m_iCount - iPosition - 1) * sizeof(T));

		// Update count
		m_iCount--;

		// Update snapshot
		CowListSnapshot<T>* pending = m_pending.Get();
		CowListSnapshot<T>* newSnapshot = BuildSnapshot(pending, ChangeKind::Deleted, &val);
		StorePendingSnapshot(newSnapshot, pending, ChangeKind::Deleted, &val);
	}

	void Move(int iFrom, int iTo)
	{
		assert(iFrom >= 0 && iFrom < GetCount());
		assert(iTo >= 0 && iTo < GetCount());

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
		CowListSnapshot<T>* pending = m_pending.Get();
		CowListSnapshot<T>* newSnapshot = BuildSnapshot(pending, ChangeKind::None, nullptr);
		StorePendingSnapshot(newSnapshot, pending, ChangeKind::None, nullptr);
	}

	// What kind of change (if any) a mutating call itself contributes to the
	// inserted/deleted lists, on top of whatever a still-pending snapshot
	// already carries forward.
	enum class ChangeKind { None, Inserted, Deleted };

	// Builds a new snapshot of the current m_pData, carrying forward
	// `pending`'s inserted/deleted lists (if any - pass nullptr for none)
	// plus this call's own single contribution (kind/val).
	CowListSnapshot<T>* BuildSnapshot(CowListSnapshot<T>* pending, ChangeKind kind, const T* val)
	{
		int insertedCount = (pending ? pending->GetInsertedCount() : 0) + (kind == ChangeKind::Inserted ? 1 : 0);
		int deletedCount = (pending ? pending->GetDeletedCount() : 0) + (kind == ChangeKind::Deleted ? 1 : 0);

		CowListSnapshot<T>* newSnapshot = new CowListSnapshot<T>(m_iCount, insertedCount, deletedCount);
		memcpy(newSnapshot->GetItemBuffer(), m_pData, sizeof(T) * m_iCount);

		if (pending)
		{
			memcpy(newSnapshot->GetInsertedBuffer(), pending->GetInsertedBuffer(), sizeof(T) * pending->GetInsertedCount());
			memcpy(newSnapshot->GetDeletedBuffer(), pending->GetDeletedBuffer(), sizeof(T) * pending->GetDeletedCount());
		}

		if (kind == ChangeKind::Inserted)
			newSnapshot->GetInsertedBuffer()[insertedCount - 1] = *val;
		else if (kind == ChangeKind::Deleted)
			newSnapshot->GetDeletedBuffer()[deletedCount - 1] = *val;

		return newSnapshot;
	}

	void StorePendingSnapshot(CowListSnapshot<T>* newSnapshot, CowListSnapshot<T>* oldSnapshot, ChangeKind kind, const T* val)
	{
		// Try to store it
		if (!m_pending.TrySet(newSnapshot, oldSnapshot))
		{
			// Under the single-writer/single-reader contract, the only way
			// this CAS can fail is the reader's GetSnapshot() concurrently
			// swapping pending out via Set(nullptr) - and pulling out a
			// non-null value (i.e. oldSnapshot) always makes GetSnapshot()
			// retire m_current. So a CAS failure here means a retirement
			// just happened that nobody has reclaimed yet, and that
			// `newSnapshot` (built by merging with oldSnapshot's now-stale
			// inserted/deleted lists) is wrong - the reader already folded
			// all of that into its new baseline.
			//
			// Reclaim the unclaimed retirement, then rebuild containing only
			// *this* call's own delta (as if pending were null) and publish
			// that instead. Nothing else can be racing m_pending at this
			// point (the reader won't touch it again until its next
			// GetSnapshot(), and there's only one writer), so a plain Set()
			// is safe here.
			ReclaimRetired();

			delete newSnapshot;
			newSnapshot = BuildSnapshot(nullptr, kind, val);
			m_pending.Set(newSnapshot);
		}
		else
		{
			// The previous snap shot was never used, discard it
			delete oldSnapshot;
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
			//
			// Pushed onto the m_retired stack rather than stored in a single
			// slot: this retire step isn't atomic with consuming `pending`
			// above (the reconciliation loop runs in between), so by the time
			// we get here an arbitrary number of writer calls may already
			// have published and been retired again since we last checked -
			// m_retired may already be non-empty, and that's fine.
			CowListSnapshot<T>* oldCurrent = m_current;
			CowListSnapshot<T>* head = m_retired.Get();
			for (;;)
			{
				oldCurrent->next = head;
				CowListSnapshot<T>* prevHead = m_retired.CompareExchange(oldCurrent, head);
				if (prevHead == head)
					break;
				head = prevHead;
			}
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
		for (int i = 0; i < GetCount(); i++)
		{
			if (GetAt(i) == arg)
				return i;
		}
		return -1;
	}

	void Remove(const T& arg)
	{
		int pos = Find(arg);
		if (pos >= 0)
			RemoveAt(pos);
	}

	int GetCount() const
	{
		return m_iCount;
	}

	T& operator[] (int index) const
	{
		return GetAt(index);
	}

	T& GetAt(int index) const
	{
		assert(index >= 0 && index < m_iCount);

		return m_pData[index];
	}

	int IndexOf(const T& item)
	{
		for (int i = 0; i < m_iCount; i++)
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
		qsort(m_pData, m_iCount, sizeof(T), (PFNCOMPAREQSORT)pfnCompare);
	}
	*/



protected:
	// Writer side mutable data
	T* m_pData;
	int m_iCapacity;
	int m_iCount;
	int m_iGrowCapacity;
	int m_iInitialCapacity;

	// Pending immutable vector will be picked up on next snapshot
	Atomic<CowListSnapshot<T>*> m_pending;

	// Current snapshot of the vector used by reader side
	CowListSnapshot<T>* m_current;

	// Snapshot retired by the reader (GetSnapshot), awaiting deletion by the
	// writer thread - see ReclaimRetired() and the freeing note up top.
	Atomic<CowListSnapshot<T>*> m_retired;
};


}