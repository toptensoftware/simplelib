#ifndef __simplelib_stringbuilder_h__
#define __simplelib_stringbuilder_h__

#include <stdlib.h>
#include <string.h>

#include "semantics.h"

namespace SimpleLib
{

// Simple StringBuilder class that uses embedded short buffer but switches
// to dynamic allocations for longer strings
template <typename T>
class CStringBuilder
{
public:
	// Constructor
	CStringBuilder()
	{
		m_iUsed = 0;
		m_pMem = m_shortBuffer;
		m_iCapacity = sizeof(m_shortBuffer) / sizeof(T);
	}

	// Destructor
	virtual ~CStringBuilder()
	{
		Reset();
	}

	// Reset the stringbuilder, releasing any dynamically allocated buffer
	void Reset()
	{
		if (m_pMem != m_shortBuffer)
			free(m_pMem);
		m_iUsed = 0;
		m_pMem = m_shortBuffer;
		m_iCapacity = sizeof(m_shortBuffer) / sizeof(T);
	}

	// Clear, keeping memory buffer
	void Clear()
	{
		m_iUsed = 0;
	}

	// Append a character
	void Append(T ch)
	{
		*Reserve(1) = ch;
	}

	// Append a null terminated string
	void Append(const T* psz)
	{
		Append(psz, SChar<T>::Length(psz));
	}

	// Append a string of specified length
	void Append(const T* psz, int length)
	{
		T* dest = Reserve(length);
		memcpy(dest, psz, sizeof(T) * length);
	}

	// Make sure the buffer is big enough for a specifed number
	// of characters and return pointer to uninitialized buffer
	// NB: This reserves space at the end of anything already
	//     allocated
	T* Reserve(int length)
	{
		if (m_iUsed + length > m_iCapacity)
		{
			// Work out new capacity
			int newCap = m_iCapacity;
			while (newCap < m_iUsed + length)
				newCap *= 2;

			if (m_pMem == m_shortBuffer)
			{
				T* pNew = (T*)malloc(sizeof(T) * newCap);
				if (pNew == nullptr)
					return nullptr;
				m_pMem = pNew;
				memcpy(m_pMem, m_shortBuffer, m_iUsed * sizeof(T));
			}
			else
			{
				T* pNew = (T*)realloc(m_pMem, sizeof(T) * newCap);
				if (pNew == nullptr)
					return nullptr;
				m_pMem = pNew;
			}
		}

		// Take room
		T* retv = m_pMem + m_iUsed;
		m_iUsed += length;
		return retv;
	}

	// Finish building and return the current string (doesn't reset the builder)
	T* Finish() const
	{
		int unused;
		return Finish(&unused);
	}

	// Finish building and return the current string and its length
	T* Finish(int* piLength) const
	{
		if (m_iUsed == 0 || (m_iUsed > 0 && m_pMem[m_iUsed - 1] != '\0'))
		{
			const_cast<CStringBuilder*>(this)->Append('\0');
		}
		*piLength = m_iUsed - 1;
		return m_pMem;
	}

	operator const T* () const
	{
		return Finish();
	}

	const T* sz() const
	{
		return Finish();
	}



private:
	T* m_pMem;
	int m_iUsed;
	int m_iCapacity;
	T m_shortBuffer[128];
};

}

#endif // __simplelib_stringbuilder_h__