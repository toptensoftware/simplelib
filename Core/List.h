#pragma once

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "Compare.h"
#include "Semantics.h"
#include "PlacedConstructor.h"
#include "Delegate.h"

#include "Allocator.h"

namespace SimpleLib
{


// List
template <typename T, typename TAllocator = TMalloc>
class List
{
	typedef typename get_semantics<T>::TSemantics TSemantics;
	typedef typename TSemantics::TArg TArg;
	typedef typename TSemantics::TStorage TStorage;

	public:
	// Constructor
	List()
	{
	}

	// Destructor
	virtual ~List()
	{
		Clear();
		if (m_data)
			TAllocator::Free(m_data);
	}

	// No copy
	List(const List&) = delete;
	List& operator=(const List&) = delete;		

	// Move
	List(List&& other)
	{
		m_count = other.m_count;
		m_capacity = other.m_capacity;
		m_data = other.m_data;

		other.m_data = nullptr;
		other.m_capacity = 0;
		other.m_count = 0;
	}

	// Move
	List& operator=(List&& other)
	{
		if (this == &other)
			return *this;

		if (m_data)
			TAllocator::Free(m_data);

		m_count = other.m_count;
		m_capacity = other.m_capacity;
		m_data = other.m_data;

		other.m_data = nullptr;
		other.m_capacity = 0;
		other.m_count = 0;

		return *this;    
	}

	// Ensure allocated capacity is at least iRequiredCapacity (does not shrink)
	bool SetCapacity(int iRequiredCapacity)
	{
		// Quit if already big enough
		if (iRequiredCapacity <= m_capacity)
			return true;

		// Work out how big to make it
		int iNewCapacity = iRequiredCapacity * 2;
		if (iNewCapacity < 16)
			iNewCapacity = 16;

		if (m_data)
		{
			// Reallocate memory
			assert(m_capacity != 0);
			m_data = (TStorage*)TAllocator::ReAlloc((void*)m_data, iNewCapacity * sizeof(TStorage));
			if (!m_data)
				return false;
		}
		else
		{
			// Allocate memory
			assert(m_capacity == 0);
			m_data = (TStorage*)TAllocator::Alloc(iNewCapacity * sizeof(TStorage));
			if (!m_data)
				return false;
		}

		// Store new capacity
		m_capacity = iNewCapacity;
		return true;
	}

	// Set the number of elements, adding default-constructed elements or
	// popping existing ones as needed
	bool SetCount(int iRequiredCount, TArg val)
	{
		if (!SetCapacity(iRequiredCount))
			return false;
		while (GetCount() < iRequiredCount)
			Add(val);
		while (GetCount() > iRequiredCount)
			Pop();
		return true;
	}

	// Release extra memory
	void FreeExtra()
	{
		// Quit if no extra memory allocated
		if (m_capacity == m_count)
			return;

		// Free or realloc memory...
		if (m_count == 0)
		{
			TAllocator::Free(m_data);
			m_data = nullptr;
		}
		else
		{
			m_data = (TStorage*)TAllocator::ReAlloc(m_data, m_count * sizeof(TStorage));
		}

		// Store new capacity
		m_capacity = m_count;
	}

	// InsertAt
	bool InsertAt(int iPosition, TArg val)
	{
		assert(iPosition >= 0);
		assert(iPosition <= GetCount());

		// Make sure have room
		if (!SetCapacity(m_count + 1))
			return false;

		// Shuffle memory
		if (iPosition < m_count)
			memmove(m_data + iPosition + 1, m_data + iPosition, (m_count - iPosition) * sizeof(TStorage));

		// Construct new element directly from TArg (same as Add)
		Constructor(m_data + iPosition, val);

		// Update count
		m_count++;
		return true;
	}

	// Add the contents of another list to the end of this one (copies each
	// element, so TStorage must be copyable)
	void AddRange(const List& vec)
	{
		InsertAtInternal(GetCount(), vec.GetBuffer(), vec.GetCount());
	}

	// Move the contents of another list to the end of this one, leaving it
	// empty. Works with move-only element types (eg. List<OwnedPtr<T>>)
	void AddRange(List&& vec)
	{
		InsertRangeAt(GetCount(), SimpleLib::move(vec));
	}

	// Insert the contents of another list into this one (copies each
	// element, so TStorage must be copyable)
	void InsertRangeAt(int iPosition, const List& vec)
	{
		InsertAtInternal(iPosition, vec.GetBuffer(), vec.GetCount());
	}

	// Insert the contents of another list into this one, transferring
	// ownership of its elements and leaving it empty. Works with move-only
	// element types (eg. List<OwnedPtr<T>>)
	bool InsertRangeAt(int iPosition, List&& vec)
	{
		assert(&vec != this);

		int iCount = vec.GetCount();
		if (iCount < 1)
			return true;

		assert(iPosition >= 0);
		assert(iPosition <= GetCount());

		// Make sure have room
		if (!SetCapacity(m_count + iCount))
			return false;

		// Shuffle memory to make room
		if (iPosition < m_count)
			memmove(m_data + iPosition + iCount, m_data + iPosition, (m_count - iPosition) * sizeof(TStorage));

		// Bitwise relocate elements straight out of vec's buffer - no
		// constructor/destructor calls, consistent with how RemoveAt and
		// InsertAtInternal already relocate elements via memmove
		memcpy(m_data + iPosition, vec.m_data, iCount * sizeof(TStorage));

		m_count += iCount;
		vec.m_count = 0;
		return true;
	}

	template <typename TColl>
	void InsertManyAt(int iPosition, const TColl& coll)
	{
		for (auto iter = coll.Iterate(); iter.Next(); )
		{
			InsertAt(iPosition++, iter.Get());
		}
	}

	template <typename TColl>
	void AddMany(const TColl& coll)
	{
		InsertManyAt(GetCount(), coll);
	}


	// ReplaceAt
	void ReplaceAt(int iPosition, TArg val)
	{
		assert(iPosition >= 0 && iPosition < GetCount());

		Destructor(m_data + iPosition);
		Constructor(m_data + iPosition, val);
	}

	// Swap two elements in the collection
	void Swap(int iPosA, int iPosB)
	{
		assert(iPosA >= 0 && iPosA < GetCount());
		assert(iPosB >= 0 && iPosB < GetCount());

		// Redundant?
		if (iPosA == iPosB)
			return;

		// Swap it
		TStorage temp = m_data[iPosA];
		Destructor(m_data + iPosA);
		Constructor(m_data + iPosA, m_data[iPosB]);
		Destructor(m_data + iPosB);
		Constructor(m_data + iPosB, temp);
	}

	// Move an element from one position to another
	void Move(int iFrom, int iTo)
	{
		assert(iFrom >= 0 && iFrom < GetCount());
		assert(iTo >= 0 && iTo < GetCount());

		// Redundant?
		if (iFrom == iTo)
			return;

		TStorage temp = m_data[iFrom];
		Destructor(m_data + iFrom);
		if (iTo < iFrom)
		{
			memmove(m_data + iTo + 1, m_data + iTo, (iFrom - iTo) * sizeof(TStorage));
		}
		else
		{
			memmove(m_data + iFrom, m_data + iFrom + 1, (iTo - iFrom) * sizeof(TStorage));
		}
		Constructor(m_data + iTo, temp);
	}

	// Add
	int Add(TArg val)
	{
		// Grow if necessary
		if (m_count + 1 > m_capacity)
		{
			if (!SetCapacity(m_count + 1))
				return -1;
		}

		Constructor(m_data + m_count, val);
		m_count++;
		return m_count - 1;
	}

	// Remove a particular item
	int Remove(TArg val)
	{
		int iPos = IndexOf(val);
		if (iPos >= 0)
			RemoveAt(iPos);
		return iPos;
	}

	// RemoveAt
	void RemoveAt(int iPosition)
	{
		assert(iPosition >= 0);
		assert(iPosition < GetCount());

		Destructor(m_data + iPosition);

		// Shuffle memory
		if (iPosition < GetCount() - 1)
			memmove(m_data + iPosition, m_data + iPosition + 1, (m_count - iPosition - 1) * sizeof(TStorage));

		// Update count
		m_count--;
	}

	TArg DetachAt(int iPosition)
	{
		assert(iPosition >= 0);
		assert(iPosition < GetCount());

		// Copy as TStorage
		TArg val = TSemantics::Detach(m_data[iPosition]);

		// Shuffle memory
		if (iPosition < GetCount() - 1)
			memmove(m_data + iPosition, m_data + iPosition + 1, (m_count - iPosition - 1) * sizeof(TStorage));

		// Update count
		m_count--;

		return val;
	}

	// Detach every element (eg. for List<OwnedPtr<T>> this abandons the
	// owned objects rather than deleting them) and empty the list
	void DetachAll()
	{
		for (int i = 0; i < m_count; i++)
			TSemantics::Detach(m_data[i]);
		m_count = 0;
	}

	// RemoveAt
	void RemoveAt(int iPosition, int iCount)
	{
		// Quit if nothing to do!
		if (iCount == 0)
			return;

		assert(iPosition >= 0);
		assert(iPosition < GetCount());
		assert(iPosition + iCount - 1 < GetCount());
		assert(m_count - iCount >= 0);

		for (int i = 0; i < iCount; i++)
		{
			Destructor(m_data + iPosition + i);
		}

		// Shuffle emory
		if (iPosition + iCount < GetCount())
			memmove(m_data + iPosition, m_data + iPosition + iCount, (m_count - (size_t)iPosition - (size_t)iCount) * sizeof(TStorage));

		// Update count
		m_count -= iCount;
	}

	// RemoveAll
	void Clear()
	{
		if (m_count)
		{
			RemoveAt(0, m_count);
			m_count = 0;
		}
	}

	// GetAt
	TArg GetAt(int iPosition) const
	{
		assert(iPosition >= 0);
		assert(iPosition < GetCount());

		return m_data[iPosition];
	}

	// Get reference to element
	const TStorage& GetRefAt(int position) const
	{
		assert(position >= 0);
		assert(position < GetCount());

		return m_data[position];
	}

	// Get reference to element
	TStorage& GetRefAt(int position)
	{
		assert(position >= 0);
		assert(position < GetCount());

		return m_data[position];
	}

	// operator[]
	TArg operator[](int iPosition) const
	{
		return GetAt(iPosition);
	}

	// GetBuffer
	TStorage* GetBuffer() const
	{
		return m_data;
	}

	// GetCount
	int GetCount() const
	{
		return m_count;
	}

    class Iter
    {
    public:
        TArg Get() { return *_value; };

        bool Next() { return _owner->GetNext(*this); }

    private:
        Iter(const List* owner, bool forward)
        {
            _owner = owner;
            _forward = forward;
        }

        Iter(const Iter& other)
        {
            _owner = other._owner;
            _forward = other._forward;
            _pos = other._pos;
            _value = other._value;
        }

        const TStorage* _value = nullptr;
        const List* _owner;
        int _pos = -1;
        bool _forward = true;
        friend class List;
    };

    Iter Iterate() const
    {
        return Iter(this, true);
    }

    Iter IterateReverse() const
    {
        Iter iter(this, false);
        iter._pos = m_count;
        return iter;
    }

	bool GetNext(Iter& iter) const
    {
        if (iter._forward)
        {
            iter._pos++;
            if (iter._pos >= m_count)
                return false;
        }
        else
        {
            iter._pos--;
            if (iter._pos < 0)
                return false;
        }

		iter._value = m_data + iter._pos;
        return true;
    }

public:
	// Binary search this list (which must already be sorted in ascending
	// order according to the same convention as compare - negative if elem
	// comes before key, 0 if equal, positive if elem comes after key). If
	// there are multiple matching elements, which one is found is unspecified.
	//
	// Implemented directly rather than via the CRT's bsearch/bsearch_s since
	// the latter isn't available in a portable form (bsearch_s is an MSVC/
	// Annex K extension with no equivalent on other platforms).
	//
	// If found:
	//   - returns `true` with `index` containing the item's index (not
	//     necessarily the first if there are duplicates)
	// If not found:
	//   - returns `false` with `index` containing the position it should be
	//     inserted at to keep the list sorted
	template <typename TKey>
	bool BinarySearch(int& index, TKey key, int (*compare)(TArg elem, TKey key, void* user), void* user = nullptr) const
	{
		int lo = 0;
		int hi = m_count - 1;
		while (lo <= hi)
		{
			int mid = lo + (hi - lo) / 2;
			int cmp = compare(m_data[mid], key, user);
			if (cmp == 0)
			{
				index = mid;
				return true;
			}
			if (cmp < 0)
				lo = mid + 1;
			else
				hi = mid - 1;
		}
		index = lo;
		return false;
	}

	// As above, but takes any callable (eg. a capturing lambda) directly so
	// no void* user needs to be threaded through. TCompare is deduced from
	// the callable itself; the enable_if excludes plain function pointers
	// matching the overload above so calls like BinarySearch(index, key,
	// CFunctionPointer) aren't ambiguous between the two overloads
	template <typename TKey, typename TCompare,
		typename = typename std::enable_if<!std::is_convertible<TCompare, int (*)(TArg, TKey, void*)>::value>::type>
	bool BinarySearch(int& index, TKey key, TCompare compare) const
	{
		int lo = 0;
		int hi = m_count - 1;
		while (lo <= hi)
		{
			int mid = lo + (hi - lo) / 2;
			int cmp = compare(m_data[mid], key);
			if (cmp == 0)
			{
				index = mid;
				return true;
			}
			if (cmp < 0)
				lo = mid + 1;
			else
				hi = mid - 1;
		}
		index = lo;
		return false;
	}

private:
	struct sort_ctx_s
	{
		int (*callback)(TArg a, TArg b, void* user);
		void* user;
	};

#ifdef _MSC_VER
	static int sort_function_s(void* pvctx, const void* a, const void* b)
#else
	static int sort_function_s(const void* a, const void* b, void* pvctx)
#endif
	{
		sort_ctx_s& ctx = *(sort_ctx_s*)pvctx;
		return ctx.callback(*(TStorage*)a, *(TStorage*)b, ctx.user);
	}

public:
	void Sort(int (*callback)(TArg a, TArg b, void* user), void* user)
	{
		sort_ctx_s ctx;
		ctx.callback = callback;
		ctx.user = user;
		#ifdef _MSC_VER
		qsort_s(m_data, m_count, sizeof(TStorage), sort_function_s, &ctx);
		#else
		qsort_r(m_data, m_count, sizeof(TStorage), sort_function_s, &ctx);
		#endif
	}

private:
	struct sort_ctx
	{
		int (*callback)(TArg a, TArg b);
	};

#ifdef _MSC_VER
	static int sort_function(void* pvctx, const void* a, const void* b)
#else
	static int sort_function(const void* a, const void* b, void* pvctx)
#endif
	{
		sort_ctx& ctx = *(sort_ctx*)pvctx;
		return ctx.callback(*(TStorage*)a, *(TStorage*)b);
	}

public:
	void Sort(int (*callback)(TArg a, TArg b))
	{
		sort_ctx ctx;
		ctx.callback = callback;
		#ifdef _MSC_VER
		qsort_s(m_data, m_count, sizeof(TStorage), sort_function, &ctx);
		#else
		qsort_r(m_data, m_count, sizeof(TStorage), sort_function, &ctx);
		#endif
	}

	// Sort using the default comparer
	template <typename TCompare = SDefaultCompare>
	void Sort()
	{
		Sort([](TArg a, TArg b) {
			return TCompare::Compare(a, b);
		});
	}

public:
	// Find index of an item(linear)
	template <typename TCompare = SDefaultCompare>
	int IndexOf(TArg val, int iStartAfter = -1) const
	{
		// Find an item
		for (int i = iStartAfter + 1; i < m_count; i++)
		{
			if (TCompare::AreEqual(static_cast<TArg>(m_data[i]), val))
				return i;
		}

		// Not found
		return -1;
	}

	// Check if the list contains an item
	bool Contains(TArg val) const
	{
		return IndexOf(val) >= 0;
	}

	// IsEmpty
	bool IsEmpty() const
	{
		return GetCount() == 0;
	}

	List Filter(Delegate<bool(TArg val)> predicate)
	{
		List r;
		for (int i=0; i<GetCount(); i++)
		{
			if (predicate(GetAt(i)))
				r.Add(GetAt(i));
		}
		return r;
	}

	// Transforms each element to a (possibly different) type, producing a
	// new list - like JavaScript's Array.prototype.map(). The result type
	// isn't deducible from a lambda argument, so it must be specified
	// explicitly at the call site, eg: list.Map<TResult>(fn)
	template <typename TResult>
	List<TResult> Map(Delegate<TResult(TArg val)> mapper)
	{
		List<TResult> r;
		for (int i=0; i<GetCount(); i++)
		{
			r.Add(mapper(GetAt(i)));
		}
		return r;
	}

	// Push
	void Push(TArg val)
	{
		Add(val);
	}

	// Pop
	TArg Pop()
	{
		return DetachAt(GetCount() - 1);
	}

	// Pop
	bool TryPop(TArg& val)
	{
		if (m_count == 0)
			return false;

		val = Pop();
		return true;
	}

	// Tail
	TArg Tail() const
	{
		return GetAt(GetCount() - 1);
	}

	// TryTail
	bool TryTail(TArg& val) const
	{
		if (m_count == 0)
			return false;
		val = Tail();
		return true;
	}

	// Head
	TArg Head() const
	{
		return GetAt(0);
	}

	// TryHead
	bool TryHead(TArg& val) const
	{
		if (m_count == 0)
			return false;
		val = Head();
		return true;
	}

	// Enqueue
	void Enqueue(TArg val)
	{
		Add(val);
	}

	// Remove and return the first item in the list
	TArg Dequeue()
	{
		return DetachAt(0);
	}

	// Remove and return the first item in the list
	bool TryDequeue(TArg& val)
	{
		if (m_count == 0)
			return false;

		val = Dequeue();
		return true;
	}

protected:
	int	m_count = 0;
	int	m_capacity = 0;
	TStorage* m_data = nullptr;

	// Insert at a position
	bool InsertAtInternal(int iPosition, const TStorage* pVal, int iCount)
	{
		if (iCount < 1)
			return true;

		assert(iPosition >= 0);
		assert(iPosition <= GetCount());

		// Make sure have room
		if (!SetCapacity(m_count + iCount))
			return false;

		// Shuffle memory
		if (iPosition < m_count)
			memmove(m_data + iPosition + iCount, m_data + iPosition, (m_count - iPosition) * sizeof(TStorage));

		// Store pointer
		for (int i = 0; i < iCount; i++)
		{
			Constructor(m_data + iPosition + i, *(pVal + i));
		}

		// Update count
		m_count += iCount;
		return true;
	}

};

}