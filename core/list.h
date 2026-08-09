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

	public:
	// Constructor
	List()
	{
	}

	// Destructor
	virtual ~List()
	{
		Clear();
		if (m_pData)
			free(m_pData);
	}

	// No copy
	List(const List&) = delete;
	List& operator=(const List&) = delete;		

	// Move
	List(List&& other)
	{
		m_iSize = other.m_iSize;
		m_iMemSize = other.m_iMemSize;
		m_pData = other.m_pData;

		other.m_pData = nullptr;
		other.m_iMemSize = 0;
		other.m_iSize = 0;
	}

	// Move
	List& operator=(List&& other)
	{
		if (this == &other)
			return *this;

		delete[] m_pData;

		m_iSize = other.m_iSize;
		m_iMemSize = other.m_iMemSize;
		m_pData = other.m_pData;

		other.m_pData = nullptr;
		other.m_iMemSize = 0;
		other.m_iSize = 0;

		return *this;    
	}

	// Reallocate memory
	void GrowTo(int iRequiredSize)
	{
		// Quit if already big enough
		if (iRequiredSize <= m_iMemSize)
			return;

		// Work out how big to make it
		int iNewSize = iRequiredSize * 2;
		if (iNewSize < 16)
			iNewSize = 16;

		if (m_pData)
		{
			// Reallocate memory
			assert(m_iMemSize != 0);
			m_pData = (T*)realloc((void*)m_pData, iNewSize * sizeof(T));
		}
		else
		{
			// Allocate memory
			assert(m_iMemSize == 0);
			m_pData = (T*)malloc(iNewSize * sizeof(T));
		}

		// Store new sizes
		m_iMemSize = iNewSize;
	}

	// Set size...
	void SetSize(int iRequiredSize, const T& val)
	{
		GrowTo(iRequiredSize);
		while (GetCount() < iRequiredSize)
			Add(val);
		while (GetCount() > iRequiredSize)
			Pop();
	}

	// Release extra memory
	void FreeExtra()
	{
		// Quit if no extra memory allocated
		if (m_iMemSize == m_iSize)
			return;

		// Free or realloc memory...
		if (m_iSize == 0)
		{
			free(m_pData);
			m_pData = nullptr;
		}
		else
		{
			m_pData = (T*)realloc(m_pData, m_iSize * sizeof(T));
		}

		// Store new memory size
		m_iMemSize = m_iSize;
	}

	// InsertAt
	void InsertAt(int iPosition, const TArg& val)
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
	void ReplaceAt(int iPosition, const TArg& val)
	{
		assert(iPosition >= 0 && iPosition < GetCount());

		Destructor(m_pData + iPosition);
		Constructor(m_pData + iPosition, val);
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
		T temp = m_pData[iPosA];
		Destructor(m_pData + iPosA);
		Constructor(m_pData + iPosA, m_pData[iPosB]);
		Destructor(m_pData + iPosB);
		Constructor(m_pData + iPosB, temp);
	}

	// Move an element from one position to another
	void Move(int iFrom, int iTo)
	{
		assert(iFrom >= 0 && iFrom < GetCount());
		assert(iTo >= 0 && iTo < GetCount());

		// Redundant?
		if (iFrom == iTo)
			return;

		T temp = m_pData[iFrom];
		Destructor(m_pData + iFrom);
		if (iTo < iFrom)
		{
			memmove(VECDATAPTR(iTo + 1), VECDATAPTR(iTo), (iFrom - iTo) * sizeof(T));
		}
		else
		{
			memmove(VECDATAPTR(iFrom), VECDATAPTR(iFrom + 1), (iTo - iFrom) * sizeof(T));
		}
		Constructor(m_pData + iTo, temp);
	}

	// Add
	int Add(const TArg& val)
	{
		// Grow if necessary
		if (m_iSize + 1 > m_iMemSize)
			GrowTo(m_iSize + 1);

		Constructor(m_pData + m_iSize, val);
		m_iSize++;
		return m_iSize - 1;
	}

	// Remove a particular item
	int Remove(const TArg& val)
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

		Destructor(m_pData + iPosition);

		// Shuffle memory
		if (iPosition < GetCount() - 1)
			memmove(VECDATAPTR(iPosition), VECDATAPTR(iPosition + 1), (m_iSize - iPosition - 1) * sizeof(T));

		// Update size
		m_iSize--;
	}

	T DetachAt(int iPosition)
	{
		assert(iPosition >= 0);
		assert(iPosition < GetCount());

		// Copy as T
		T val = move(m_pData[iPosition]);

		// Shuffle memory
		if (iPosition < GetCount() - 1)
			memmove(VECDATAPTR(iPosition), VECDATAPTR(iPosition + 1), (m_iSize - iPosition - 1) * sizeof(T));

		// Update size
		m_iSize--;

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
		assert(m_iSize - iCount >= 0);

		for (int i = 0; i < iCount; i++)
		{
			Destructor(m_pData + iPosition + i);
		}

		// Shuffle emory
		if (iPosition + iCount < GetCount())
			memmove(VECDATAPTR(iPosition), VECDATAPTR(iPosition + iCount), (m_iSize - (size_t)iPosition - (size_t)iCount) * sizeof(T));

		// Update size
		m_iSize -= iCount;
	}

	// RemoveAll
	void Clear()
	{
		if (m_iSize)
		{
			RemoveAt(0, m_iSize);
			m_iSize = 0;
		}
	}

	// GetAt
	TArg& GetAt(int iPosition) const
	{
		assert(iPosition >= 0);
		assert(iPosition < GetCount());

		return m_pData[iPosition];
	}

	// operator[]
	TArg& operator[](int iPosition) const
	{
		return GetAt(iPosition);
	}

	// GetBuffer
	T* GetBuffer() const
	{
		return m_pData;
	}

	// GetCount
	int GetCount() const
	{
		return m_iSize;
	}

    class Iter
    {
    public:
        const T& Get() { return *_value; };

        bool Next() { return _owner->GetNext(*this); }

    private:
        Iter(const List* owner)
        {
            _owner = owner;
        }

        Iter(const Iter& other)
        {
            _owner = other._owner;
            _pos = other._pos;
            _value = other._value;
        }

        const T* _value = nullptr;
        const List* _owner;
        int _pos = -1;
        friend class List;
    };

    Iter Iterate() const
    {
        return Iter(this);
    }

	bool GetNext(Iter& iter) const
    {
        iter._pos++;
		if (iter._pos >= m_iSize)
			return false;

		iter._value = m_pData + iter._pos;
        return true;
    }

	struct sort_ctx_s
	{
		int (*callback)(const T& a, const T& b, void* user);
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

	void Sort(int (*callback)(const T& a, const T& b, void* user), void* user)
	{
		sort_ctx_s ctx;
		ctx.callback = callback;
		ctx.user = user;
		#ifdef _MSC_VER
		qsort_s(m_pData, m_iSize, sizeof(T), sort_function_s, &ctx);
		#else
		qsort_r(m_pData, m_iSize, sizeof(T), sort_function_s, &ctx);
		#endif
	}

	struct sort_ctx
	{
		int (*callback)(const T& a, const T& b);
	};

#ifdef _MSC_VER
	static int sort_function(void* pvctx, const void* a, const void* b)
#else
	static int sort_function(const void* a, const void* b, void* pvctx)
#endif
	{
		sort_ctx& ctx = *(sort_ctx*)pvctx;
		return ctx.callback(*(T*)a, *(T*)b);
	}

	void Sort(int (*callback)(const T& a, const T& b))
	{
		sort_ctx ctx;
		ctx.callback = callback;
		#ifdef _MSC_VER
		qsort_s(m_pData, m_iSize, sizeof(T), sort_function, &ctx);
		#else
		qsort_r(m_pData, m_iSize, sizeof(T), sort_function, &ctx);
		#endif
	}

	// Find index of an item(linear)
	template <typename TCompare = SDefaultCompare>
	int IndexOf(const TArg& val, int iStartAfter = -1) const
	{
		// Find an item
		for (int i = iStartAfter + 1; i < m_iSize; i++)
		{
			if (TCompare::AreEqual(m_pData[i], val))
				return i;
		}

		// Not found
		return -1;
	}

	// Check if the list contains an item
	bool Contains(const TArg& val) const
	{
		return IndexOf(val) >= 0;
	}

	// IsEmpty
	bool IsEmpty() const
	{
		return GetCount() == 0;
	}

	List Filter(Delegate<bool(const T& val)> predicate)
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
	List<TResult> Map(Delegate<TResult(const T& val)> mapper)
	{
		List<TResult> r;
		for (int i=0; i<GetCount(); i++)
		{
			r.Add(mapper(GetAt(i)));
		}
		return r;
	}

	// Push
	void Push(const TArg& val)
	{
		Add(val);
	}

	// Pop
	TArg Pop()
	{
		assert(!IsEmpty());
		T temp = GetAt(GetCount() - 1);
		RemoveAt(GetCount() - 1);
		return temp;
	}

	// Pop
	bool TryPop(TArg& val)
	{
		if (m_iSize == 0)
			return false;

		// Update size
		m_iSize--;

		val = m_pData[m_iSize];

		Destructor(m_pData + m_iSize);

		return true;
	}

	// Tail 
	TArg& Tail() const
	{
		assert(!IsEmpty());
		return GetAt(GetCount() - 1);
	}

	// TryTail
	bool TryTail(TArg& val) const
	{
		if (IsEmpty())
			return false;
		val = Tail();
		return true;
	}

	// Head
	TArg& Head() const
	{
		assert(!IsEmpty());
		return GetAt(0);
	}

	// TryHead
	bool TryHead(TArg& val) const
	{
		if (IsEmpty())
			return false;
		val = Head();
		return true;
	}

	// Enqueue
	void Enqueue(const TArg& val)
	{
		Add(val);
	}

	// Remove and return the first item in the list
	TArg Dequeue()
	{
		assert(!IsEmpty());
		T temp = GetAt(0);
		RemoveAt(0);
		return temp;
	}

	// Remove and return the first item in the list
	bool TryDequeue(TArg& val)
	{
		if (IsEmpty())
			return false;

		val = Dequeue();
		return true;
	}

protected:
	int		m_iSize = 0;
	int		m_iMemSize = 0;
	T* 		m_pData = nullptr;

	// Insert at a position
	void InsertAtInternal(int iPosition, const T* pVal, int iCount)
	{
		if (iCount < 1)
			return;

		assert(iPosition >= 0);
		assert(iPosition <= GetCount());

		// Make sure have room
		GrowTo(m_iSize + iCount);

		// Shuffle memory
		if (iPosition < m_iSize)
			memmove(VECDATAPTR(iPosition + iCount), VECDATAPTR(iPosition), (m_iSize - iPosition) * sizeof(T));

		// Store pointer
		for (int i = 0; i < iCount; i++)
		{
			Constructor(m_pData + iPosition + i, *(pVal + i));
		}

		// Update size
		m_iSize += iCount;
	}

	void* VECDATAPTR(int index) { return (void*)(((char*)m_pData) + sizeof(m_pData[0]) * (index)); }

};

}