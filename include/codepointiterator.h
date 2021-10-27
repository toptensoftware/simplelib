#ifndef __simplelib_codepointiterator_h__
#define __simplelib_codepointiterator_h__

#include "encoding.h"

namespace SimpleLib
{

template <typename T, typename TEncoding>
struct CCodePointIteratorEncoded
{
	// Constructor
	CCodePointIteratorEncoded(const T* p, int length)
	{
		m_pStart = p;
		m_pPosition = p;
		m_pPositionNext = p;
		m_ch = TEncoding::Decode(m_pPositionNext);
		m_iIndex = 0;
		m_pEnd = nullptr;
		m_iCount = 0;
	}

	// Advance by N code points
	bool Advance(int codePoints)
	{
		while (codePoints--)
		{
			if (m_ch == 0)
				return false;

			m_pPosition = m_pPositionNext;
			m_ch = TEncoding::Decode(m_pPositionNext);
			m_iIndex++;

			if (m_ch == 0)
			{
				m_pEnd = m_pPosition;
				m_iCount = m_iIndex;
			}
		}
		return true;
	}

	// Rewind by N code points
	bool Rewind(int codePoints)
	{
		if (codePoints == 0)
			return true;
		while (codePoints--)
		{
			if (m_pPosition <= m_pStart)
			{
				m_pPosition = m_pStart;
				m_iIndex = 0;
				break;
			}

			m_pPosition = TEncoding::Rewind(m_pPosition);
			m_iIndex--;
		}

		m_pPositionNext = m_pPosition;
		m_ch = TEncoding::Decode(m_pPositionNext);
		return true;
	}

	void FindEnd()
	{
		// Keep advancing until we hit the end
		while (m_pEnd == nullptr)
		{
			Advance(0x7FFFFFFF);
		}
	}

	// Move to specified code point index
	bool MoveTo(int codePointIndex)
	{
		if (codePointIndex == m_iIndex)
			return true;

		if (codePointIndex == 0)
		{
			// Easy case
			m_pPosition = m_pStart;
		}
		else if (m_pEnd != nullptr && codePointIndex == m_iCount)
		{
			// Easy case
			m_pPosition = m_pEnd;
		}
		else
		{
			int distanceFromStart = codePointIndex;
			int distanceFromCurrent = abs(codePointIndex - m_iIndex);

			// Closer to start?
			if (distanceFromStart < distanceFromCurrent)
			{
				MoveTo(0);
				Advance(codePointIndex);
				return m_iIndex == codePointIndex;
			}

			// End known and closer to end?
			if (m_pEnd != nullptr)
			{
				int distanceFromEnd = m_iCount - codePointIndex;
				if (distanceFromEnd < distanceFromCurrent)
				{
					MoveTo(m_iCount);
					Rewind(distanceFromEnd);
					return m_iIndex == codePointIndex;
				}
			}

			// Move relative to current position
			if (codePointIndex > m_iIndex)
				Advance(codePointIndex - m_iIndex);
			else
				Rewind(m_iIndex - codePointIndex);
			return m_iIndex == codePointIndex;
		}

		// Update position
		m_pPositionNext = m_pPosition;
		m_ch = TEncoding::Decode(m_pPositionNext);
		m_iIndex = codePointIndex;
		return true;
	}

	char32_t GetCodePointAt(int codePointIndex)
	{
		if (MoveTo(codePointIndex))
			return m_ch;
		else
			return '\0';
	}

	// Get the code point at the current position
	char32_t GetCodePoint()
	{
		return m_ch;
	}

	// Get the index of the current code point
	int GetCodePointIndex()
	{
		return m_iIndex;
	}

	// Get the number of code points
	int GetCodePointCount()
	{
		FindEnd();
		return m_iCount;
	}

private:
	const T* m_pStart;
	const T* m_pPosition;
	const T* m_pPositionNext;
	char32_t m_ch;
	int m_iIndex;
	const T* m_pEnd;
	int m_iCount;
};

template <typename T>
struct CCodePointIteratorUnencoded
{
	// Constructor
	CCodePointIteratorUnencoded(const T* p, int length)
	{
		m_pStart = p;
		m_iIndex = 0;
		m_iCount = length;
		if (m_iCount < 0)
		{
			while (*p)
				p++;
			m_iCount = p - m_pStart;
		}
	}

	// Advance by N code points
	bool Advance(int codePoints)
	{
		m_iIndex = Min(m_iCount, m_iIndex + codePoints);
	}

	// Rewind by N code points
	bool Rewind(int codePoints)
	{
		m_iIndex = Max(0, m_iIndex - codePoints);
	}

	// Move to specified code point index
	bool MoveTo(int codePointIndex)
	{
		m_iIndex = codePointIndex;
		if (m_iIndex < 0)
			m_iIndex = 0;
		if (m_iIndex >= m_iCount)
			m_iIndex = m_iCount;
		return m_iIndex == codePointIndex;
	}

	// Get the code point at the current position
	char32_t GetCodePoint()
	{
		m_pStart(m_iIndex);
	}

	// Get the index of the current code point
	int GetCodePointIndex()
	{
		return m_iIndex;
	}

	// Get the number of code points
	int GetCodePointCount()
	{
		return m_iCount;
	}

private:
	const T* m_pStart;
	int m_iIndex;
	int m_iCount;
};

template <typename T, int size>
struct CCodePointIteratorSelector : public CCodePointIteratorEncoded<T, CEncoding<T>> 
{
	typedef CCodePointIteratorEncoded<T, CEncoding<T>> _base;
	CCodePointIteratorSelector(const T* psz, int length) : _base(psz, length)
	{
	}
};

template <typename T>
struct CCodePointIteratorSelector<T, sizeof(char32_t)> : public CCodePointIteratorUnencoded<T> 
{
	typedef CCodePointIteratorUnencoded<T> _base;
	CCodePointIteratorSelector(const T* psz, int length) : _base(psz, length)
	{
	}
};

template <typename T>
struct CCodePointIterator : public CCodePointIteratorSelector<T, sizeof(T)> 
{
	typedef CCodePointIteratorSelector<T, sizeof(T)> _base;

	CCodePointIterator(const T* psz, int length = -1) : _base(psz, length)
	{
	}
};

} // namespace

#endif  // __simplelib_codepointiterator_h__