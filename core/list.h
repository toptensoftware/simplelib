#pragma once

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "Compare.h"
#include "Semantics.h"
#include "PlacedConstructor.h"
#include "Delegate.h"


namespace SimpleLib
{


// List
template <typename T>
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
			free(m_data);
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

		delete[] m_data;

		m_count = other.m_count;
		m_capacity = other.m_capacity;
		m_data = other.m_data;

		other.m_data = nullptr;
		other.m_capacity = 0;
		other.m_count = 0;

		return *this;    
	}

	// Ensure allocated capacity is at least iRequiredCapacity (does not shrink)
	void SetCapacity(int iRequiredCapacity)
	{
		// Quit if already big enough
		if (iRequiredCapacity <= m_capacity)
			return;

		// Work out how big to make it
		int iNewCapacity = iRequiredCapacity * 2;
		if (iNewCapacity < 16)
			iNewCapacity = 16;

		if (m_data)
		{
			// Reallocate memory
			assert(m_capacity != 0);
			m_data = (TStorage*)realloc((void*)m_data, iNewCapacity * sizeof(TStorage));
		}
		else
		{
			// Allocate memory
			assert(m_capacity == 0);
			m_data = (TStorage*)malloc(iNewCapacity * sizeof(TStorage));
		}

		// Store new capacity
		m_capacity = iNewCapacity;
	}

	// Set the number of elements, adding default-constructed elements or
	// popping existing ones as needed
	void SetCount(int iRequiredCount, TArg val)
	{
		SetCapacity(iRequiredCount);
		while (GetCount() < iRequiredCount)
			Add(val);
		while (GetCount() > iRequiredCount)
			Pop();
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
			free(m_data);
			m_data = nullptr;
		}
		else
		{
			m_data = (TStorage*)realloc(m_data, m_count * sizeof(TStorage));
		}

		// Store new capacity
		m_capacity = m_count;
	}

	// InsertAt
	void InsertAt(int iPosition, TArg val)
	{
		InsertAtInternal(iPosition, &val, 1);
	}

	// Add the contents of another list to the end of this one
	void AddRange(const List& vec)
	{
		InsertAtInternal(GetCount(), vec.GetBuffer(), vec.GetCount());
	}

	// Insert the contents of another list into this one
	void InsertRangeAt(int iPosition, const List& vec)
	{
		InsertAtInternal(iPosition, vec.GetBuffer(), vec.GetCount());
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
			SetCapacity(m_count + 1);

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

	// Find index of an item(linear)
	template <typename TCompare = SDefaultCompare>
	int IndexOf(TArg val, int iStartAfter = -1) const
	{
		// Find an item
		for (int i = iStartAfter + 1; i < m_count; i++)
		{
			if (TCompare::AreEqual(m_data[i], val))
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
	void InsertAtInternal(int iPosition, const TStorage* pVal, int iCount)
	{
		if (iCount < 1)
			return;

		assert(iPosition >= 0);
		assert(iPosition <= GetCount());

		// Make sure have room
		SetCapacity(m_count + iCount);

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
	}

};

}