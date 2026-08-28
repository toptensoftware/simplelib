#pragma once

#include <stdlib.h>

#include "../core/Compare.h"

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
// Unlike CowListWops<T>, snapshots here only expose the full current
// contents (GetCount()/GetAt(i)) - there's no inserted/deleted item
// tracking. If you need to know which items were added/removed since the
// reader's previous snapshot, use CowListWops<T> instead.
//
// StartUpdate()/EndUpdate() let the writer group several mutating calls
// (Add/InsertAt/RemoveAt/Move, in any combination) into a single unit that
// becomes visible to the reader all at once. Without them, each mutating
// call publishes its own complete snapshot immediately - correct in
// isolation, but it means a GetSnapshot() that lands between two of those
// calls (the reader thread is running concurrently, so this is possible at
// any time) sees a torn, partially-applied batch. Calls nest - only the
// outermost StartUpdate()/EndUpdate() pair actually defers/publishes.
//
// Threading note: the writer builds each new snapshot from scratch off
// m_pData and publishes it via a compare-exchange against whatever
// `pending` currently holds - a failed CAS just means the reader concurrently
// consumed the old pending via GetSnapshot(), so the writer can safely
// overwrite it with a plain Set() instead. That argument depends entirely on
// the strict single-writer/single-reader pattern above - it does not
// generalize to any other usage, so don't reuse this class outside that
// pattern without re-deriving the proof.
//
// Freeing note: GetSnapshot() runs on the audio thread and must not call
// into the heap allocator. So rather than deleting the outgoing m_current
// there, it's pushed onto the m_retired stack, which the writer thread
// frees off at the start of its next mutating call. m_retired is a
// lock-free (Treiber) stack, not a single slot: GetSnapshot()'s retire step
// (consume `pending`, then push the old m_current) isn't atomic as a whole,
// so an arbitrary number of writer mutating calls can complete in that
// window - m_retired has to be able to hold every retirement that lands
// before the writer next reclaims, not just the most recent one.
//
// T must be trivially copyable (this is enforced by a static_assert) -
// CowList is designed for small POD types / raw pointers, not owning
// objects. Find/IndexOf/Remove additionally require T to be == comparable,
// but (like any template member function) only if actually called - a
// non-comparable T just means don't call those.

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

	T GetAt(int index) const
	{
		assert(index >= 0 && index < count);
		return data[index];
	}

	T operator[] (int index) const
	{
		return GetAt(index);
	}

private:
	CowListSnapshot(int count) :
		count(count)
	{
		data = (T*)malloc(sizeof(T) * count);
	}

	~CowListSnapshot()
	{
		free(data);
	}

	// Intrusive link for the writer-reclaimed retirement stack (see
	// CowList::m_retired) - GetSnapshot()'s own retire step isn't atomic
	// (there's a compare-exchange loop that pushes onto m_retired), so an
	// arbitrary number of writer mutating calls can complete in that window
	// before the writer gets a chance to reclaim. m_retired must therefore
	// be able to hold more than one outstanding retirement, not just the
	// most recent.
	CowListSnapshot<T>* next = nullptr;

	T* GetItemBuffer()
	{
		return data;
	}

	int count;
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
		m_current = new CowListSnapshot<T>(0);
		m_iUpdateDepth = 0;
		m_bBatchDirty = false;
	}

	~CowList()
	{
		Reset();
		delete m_current;
	}

	// Must not be called while a StartUpdate()/EndUpdate() batch is open -
	// there's no sensible way to reconcile an in-progress batch with a reset.
	void Reset()
	{
		assert(m_iUpdateDepth == 0);
		Clear();
		free(m_pData);
		m_pData = NULL;
		m_iCapacity = 0;
		m_iCount = 0;
		delete m_pending.Set(nullptr);
		ReclaimRetired();
		delete m_current;
		m_current = new CowListSnapshot<T>(0);
	}

	// Frees whatever snapshot(s) GetSnapshot() retired on the reader thread
	// since our last mutating call. Must run on the writer thread, at the
	// start of every mutating operation, before anything else.
	//
	// m_retired is a lock-free stack (Treiber stack), not a single slot:
	// GetSnapshot()'s retire step isn't atomic (there's a compare-exchange
	// loop between consuming `pending` and actually pushing the retired
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

	void Clear()
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

		// Suppressed during a batch - see StartUpdate()
		if (m_iUpdateDepth == 0)
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
		if (m_iUpdateDepth == 0)
			PublishSnapshot();
		else
			m_bBatchDirty = true;

		return iPosition;
	}

	void RemoveAt(int iPosition)
	{
		assert(iPosition >= 0);
		assert(iPosition < GetCount());

		// Suppressed during a batch - see StartUpdate()
		if (m_iUpdateDepth == 0)
			ReclaimRetired();

		// Shuffle memory
		if (iPosition < GetCount() - 1)
			memmove(m_pData + iPosition, m_pData + iPosition + 1, (m_iCount - iPosition - 1) * sizeof(T));

		// Update count
		m_iCount--;

		// Update snapshot
		if (m_iUpdateDepth == 0)
			PublishSnapshot();
		else
			m_bBatchDirty = true;
	}

	void Move(int iFrom, int iTo)
	{
		assert(iFrom >= 0 && iFrom < GetCount());
		assert(iTo >= 0 && iTo < GetCount());

		// Redundant?
		if (iFrom == iTo)
			return;

		// Suppressed during a batch - see StartUpdate()
		if (m_iUpdateDepth == 0)
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
		if (m_iUpdateDepth == 0)
			PublishSnapshot();
		else
			m_bBatchDirty = true;
	}

private:
	struct sort_ctx_s
	{
		int (*callback)(T a, T b, void* user);
		void* user;
	};

#ifdef _MSC_VER
	static int sort_function_s(void* pvctx, const void* a, const void* b)
#else
	static int sort_function_s(const void* a, const void* b, void* pvctx)
#endif
	{
		sort_ctx_s& ctx = *(sort_ctx_s*)pvctx;
		return ctx.callback(*(T*)a, *(T*)b, ctx.user);
	}

public:
	// Sorts the list in place using `callback`. Like Move(), this reorders
	// items, so it's wrapped in a batch (StartUpdate/EndUpdate) rather than
	// hand-rolling the dirty-marking/publish logic Move() uses directly - a
	// sort touches the whole array, not a single pair of slots, so there's
	// no cheaper single-call path worth special-casing here.
	void Sort(int (*callback)(T a, T b, void* user), void* user)
	{
		StartUpdate();

		sort_ctx_s ctx;
		ctx.callback = callback;
		ctx.user = user;
#ifdef _MSC_VER
		qsort_s(m_pData, m_iCount, sizeof(T), sort_function_s, &ctx);
#else
		qsort_r(m_pData, m_iCount, sizeof(T), sort_function_s, &ctx);
#endif
		m_bBatchDirty = true;

		EndUpdate();
	}

	void Sort(int (*callback)(T a, T b))
	{
		Sort([](T a, T b, void* user) {
			auto cb = (int (*)(T, T))user;
			return cb(a, b);
		}, (void*)callback);
	}

	// Sort using the default comparer
	template <typename TCompare = SDefaultCompare>
	void Sort()
	{
		Sort([](T a, T b) {
			return TCompare::Compare(a, b);
		});
	}

	// Starts (or, if already inside one, extends) a batch of mutating calls
	// whose combined effect becomes visible to the reader atomically - only
	// the outermost StartUpdate()/EndUpdate() pair actually defers/publishes,
	// so it's safe to wrap already-batched code in another layer of these.
	void StartUpdate()
	{
		assert(m_iUpdateDepth >= 0);
		if (m_iUpdateDepth == 0)
		{
			// Reclaim anything retired before the batch started. Once the
			// batch is open, mutating calls must NOT reclaim.
			ReclaimRetired();
			m_bBatchDirty = false;
		}
		m_iUpdateDepth++;
	}

	// Ends (or, if nested, un-nests) a batch started with StartUpdate(). On
	// the outermost call, if anything actually changed, publishes a single
	// snapshot covering the whole batch.
	void EndUpdate()
	{
		assert(m_iUpdateDepth > 0);
		if (--m_iUpdateDepth > 0)
			return;

		if (m_bBatchDirty)
			PublishSnapshot();
	}

	CowListSnapshot<T>& GetSnapshot()
	{
		CowListSnapshot<T>* pending = m_pending.Set(nullptr);
		if (pending)
		{
			// Retire the current one and replace with the pending one. Must not
			// delete m_current here - this runs on the audio thread. Hand it
			// off for the writer thread to free instead (see ReclaimRetired).
			//
			// Pushed onto the m_retired stack rather than stored in a single
			// slot: this retire step isn't atomic with consuming `pending`
			// above, so by the time we get here an arbitrary number of writer
			// calls may already have published and been retired again since
			// we last checked - m_retired may already be non-empty, and
			// that's fine.
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



private:
	// Builds a fresh snapshot of the current m_pData and publishes it to
	// m_pending, retrying against whatever the reader concurrently consumed
	// (see GetSnapshot()) - a failed CAS just means the reader already took
	// the old pending, so there's nothing left to merge and a plain Set()
	// is safe.
	void PublishSnapshot()
	{
		CowListSnapshot<T>* pending = m_pending.Get();
		CowListSnapshot<T>* newSnapshot = BuildSnapshot();

		if (!m_pending.TrySet(newSnapshot, pending))
		{
			// The reader concurrently consumed `pending` via GetSnapshot() -
			// nothing else can be racing m_pending at this point (single
			// writer, and the reader won't touch it again until its next
			// GetSnapshot()), so a plain Set() is safe here.
			m_pending.Set(newSnapshot);
		}
		else
		{
			// The previous pending snapshot was never used, discard it
			delete pending;
		}
	}

	CowListSnapshot<T>* BuildSnapshot()
	{
		CowListSnapshot<T>* newSnapshot = new CowListSnapshot<T>(m_iCount);
		memcpy(newSnapshot->GetItemBuffer(), m_pData, sizeof(T) * m_iCount);
		return newSnapshot;
	}

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

	// StartUpdate()/EndUpdate() batching state - writer-thread-only, never
	// visible to the reader.
	int m_iUpdateDepth;
	bool m_bBatchDirty;
};


}
