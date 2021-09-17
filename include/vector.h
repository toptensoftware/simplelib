#ifndef __simplelib_vector_h__
#define __simplelib_vector_h__

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "semantics.h"

namespace SimpleLib
{
	// CVector
	template <typename T, typename TSem = typename SDefaultSemantics<T>::TSemantics>
	class CVector
	{
	public:

		typedef typename SArgType<T>::TArgType TArg;
		typedef CVector<T, TSem>	_CVector;


		// Constructor
		CVector()
		{
			m_pData = nullptr;
			m_iSize = 0;
			m_iMemSize = 0;
		}

		// Destructor
		virtual ~CVector()
		{
			Clear();
			if (m_pData)
				free(m_pData);
		}

		void* VECDATAPTR(int index) { return (void*)(((char*)m_pData) + sizeof(m_pData[0]) * (index)); }


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
		void SetSize(int iRequiredSize, TArg val)
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
		void InsertAt(int iPosition, const T& val)
		{
			InsertAtInternal(iPosition, &val, 1);
		}

		// Add the contents of another vector to the end of this one
		void Add(CVector& vec)
		{
			InsertAtInternal(GetCount(), vec.GetBuffer(), vec.GetCount());
		}

		// Insert the contents of another vector into this one
		void InsertAt(int iPosition, CVector& vec)
		{
			InsertAtInternal(iPosition, vec.GetBuffer(), vec.GetCount());
		}

		// ReplaceAt
		void ReplaceAt(int iPosition, const T& val)
		{
			assert(iPosition >= 0 && iPosition < GetCount());

			TSem::TStorage::OnRemove(m_pData[iPosition], this);
			Destructor(m_pData + iPosition);
			Constructor(m_pData + iPosition, TSem::TStorage::OnAdd(val, this));
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
		int Add(const T& val)
		{
			// Grow if necessary
			if (m_iSize + 1 > m_iMemSize)
				GrowTo(m_iSize + 1);

			Constructor(m_pData + m_iSize, TSem::TStorage::OnAdd(val, this));
			m_iSize++;
			return m_iSize - 1;
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

			TSem::TStorage::OnRemove(m_pData[iPosition], this);
			Destructor(m_pData + iPosition);

			// Shuffle memory
			if (iPosition < GetCount() - 1)
				memmove(VECDATAPTR(iPosition), VECDATAPTR(iPosition + 1), (m_iSize - iPosition - 1) * sizeof(T));

			// Update size
			m_iSize--;
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
				TSem::TStorage::OnRemove(m_pData[iPosition + i], this);
				Destructor(m_pData + iPosition + i);
			}

			// Shuffle emory
			if (iPosition + iCount < GetCount())
				memmove(VECDATAPTR(iPosition), VECDATAPTR(iPosition + iCount), (m_iSize - (size_t)iPosition - (size_t)iCount) * sizeof(T));

			// Update size
			m_iSize -= iCount;
		}

		// DetachAt
		T DetachAt(int iPosition)
		{
			assert(iPosition >= 0);
			assert(iPosition < GetCount());

			T temp = GetAt(iPosition);

			TSem::TStorage::OnDetach(m_pData[iPosition], this);
			Destructor(m_pData + iPosition);

			// Shuffle memory
			if (iPosition < GetCount() - 1)
				memmove(VECDATAPTR(iPosition), VECDATAPTR(iPosition + 1), (m_iSize - iPosition - 1) * sizeof(T));

			// Update size
			m_iSize--;

			return temp;
		}

		// Detach the specified item from the collection
		void Detach(TArg val)
		{
			int iIndex = IndexOf(val);
			assert(iIndex >= 0);
			DetachAt(iIndex);
		}

		// Detach all items from the collection
		void DetachAll()
		{
			for (int i = GetCount() - 1; i >= 0; i--)
				DetachAt(i);
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
		T& GetAt(int iPosition) const
		{
			assert(iPosition >= 0);
			assert(iPosition < GetCount());

			return m_pData[iPosition];
		}

		// operator[]
		T& operator[](int iPosition) const
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

		// Find index of an item(linear)
		template <typename TEquality=typename TSem::TCompare>
		int IndexOf(TArg val, int iStartAfter = -1) const
		{
			// Find an item
			for (int i = iStartAfter + 1; i < m_iSize; i++)
			{
				if (TEquality::AreEqual(m_pData[i], val))
					return i;
			}

			// Not found
			return -1;
		}

		// Check if the vector contains an item
		bool Contains(TArg val) const
		{
			return IndexOf(val) >= 0;
		}

		// IsEmpty
		bool IsEmpty() const
		{
			return GetCount() == 0;
		}

		// Push
		void Push(TArg val)
		{
			Add(val);
		}

		// Pop
		T Pop()
		{
			assert(!IsEmpty());
			return DetachAt(GetCount() - 1);
		}

		// Pop
		bool TryPop(T& val)
		{
			if (m_iSize == 0)
				return false;

			// Update size
			m_iSize--;

			val = m_pData[m_iSize];

			TSem::TStorage::OnDetach(m_pData[m_iSize], this);
			Destructor(m_pData + m_iSize);

			return true;
		}

		// Tail 
		T& Tail() const
		{
			assert(!IsEmpty());
			return GetAt(GetCount() - 1);
		}

		// TryTail
		bool TryTail(T& val) const
		{
			if (IsEmpty())
				return false;
			val = Tail();
			return true;
		}

		// Head
		T& Head() const
		{
			assert(!IsEmpty());
			return GetAt(0);
		}

		// TryHead
		bool TryHead(T& val) const
		{
			if (IsEmpty())
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
		T Dequeue()
		{
			assert(!IsEmpty());
			return DetachAt(0);
		}

		// Remove and return the first item in the list
		bool TryDequeue(T& val)
		{
			if (IsEmpty())
				return false;

			val = Dequeue();
			return true;
		}

	protected:
		int		m_iSize;
		int		m_iMemSize;
		T* 		m_pData;

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
				Constructor(m_pData + iPosition + i, TSem::TStorage::OnAdd(*(pVal + i), this));
			}

			// Update size
			m_iSize += iCount;
		}

	private:
		// Unsupported
		CVector(const CVector& Other);
		CVector& operator=(const CVector& Other);
	};

}	// namespace

#endif  // __simplelib_vector_h__