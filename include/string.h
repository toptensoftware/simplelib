#ifndef __simplelib_string_h__
#define __simplelib_string_h__

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>

#include "vector.h"
#include "stringbuilder.h"

namespace SimpleLib
{
	/*
	Simple immutable string class stores a string.
	*/

	template <typename T>
	class CCoreString
	{
	public:
		// Constructor
		CCoreString()
		{
			m_pData = nullptr;
		}

		// Copy Constructor
		CCoreString(const CCoreString<T>& other)
		{
			m_pData = nullptr;
			Assign(other);
		}

		// Move Constructor
		CCoreString(CCoreString<T>&& other)
		{
			m_pData = other.m_pData;
			other.m_pData = nullptr;
		}

		// Constructor
		CCoreString(const T* psz, int iLen)
		{
			m_pData = AllocCoreStringData(psz, iLen);
		}

		// Constructor
		CCoreString(const T* psz)
		{
			m_pData = AllocCoreStringData(psz, -1);
		}

		// Constructor
		CCoreString(const CCoreStringBuilder<T>& builder)
		{
			int length;
			const T* psz = builder.ToString(&length);
			m_pData = AllocCoreStringData(psz, length);
		}

		// Destructor
		~CCoreString()
		{
			Clear();
		}

		// Types
		typedef CCoreString<T> _CCoreString;

		// Assignment
		CCoreString<T>& operator=(const CCoreString<T>& Other)
		{
			Assign(Other);
			return *this;
		}

		// Move Assignment
		CCoreString<T>& operator=(CCoreString<T>&& Other)
		{
			m_pData = Other.m_pData;
			Other.m_pData = nullptr;
			return *this;
		}

		// Assignment
		CCoreString<T>& operator=(const T* psz) 
		{
			Assign(psz, -1);
			return *this;
		}

		// operator[]
		const T operator[] (int iPos)
		{
			assert(m_pData);
			assert(iPos >= 0 && iPos < m_pData->m_iLength);
			return m_pData->m_sz[iPos];
		}

		// const T* operator
		operator const T* () const
		{
			return sz();
		}

		bool operator ==(const CCoreString<T>& b) const
		{
			return this->IsEqualTo(b.sz());
		}

		// Get null terminated string
		const T* sz() const
		{
			return m_pData ? m_pData->m_sz : nullptr;
		}

		void Clear()
		{
			if (m_pData)
			{
				assert(m_pData->m_sz[m_pData->m_iLength] == '\0');
				m_pData->m_iRef--;
				if (m_pData->m_iRef == 0)
				{
					free(m_pData);
				}
				m_pData = nullptr;
			}
		}

		bool IsEmpty() const
		{
			return GetLength() == 0;
		}

		bool Assign(const CCoreString<T>& Other)
		{
			Clear();
			m_pData = Other.m_pData;
			if (m_pData)
				m_pData->m_iRef++;
			return true;
		}

		void Assign(const T* psz, int iLen = -1)
		{
			// Clear old value
			Clear();

			// Store new
			m_pData = AllocCoreStringData(psz, iLen);
		}

		int GetLength() const
		{
			return m_pData ? m_pData->m_iLength : 0;
		}

		CCoreString<T> ToUpper()
		{
			if (m_pData == nullptr)
				return CCoreString<T>();

			// Allocate new string buffer
			StringData* pNew = AllocCoreStringData(nullptr, GetLength());

			// Get source/dest
			const T* pSrc = sz();
			T* pDest = pNew->m_sz;

			// Copy and to upper
			int length = GetLength();
			for (int i=0; i<length; i++)
			{
				*pDest++ = SChar<T>::ToUpper(*pSrc++);
			}

			// Return new string
			return CCoreString<T>(pNew);
		}

		CCoreString<T> ToLower()
		{
			if (m_pData == nullptr)
				return CCoreString<T>();

			// Allocate new string buffer
			StringData* pNew = AllocCoreStringData(nullptr, GetLength());

			// Get source/dest
			const T* pSrc = sz();
			T* pDest = pNew->m_sz;

			// Copy and to upper
			int length = GetLength();
			for (int i=0; i<length; i++)
			{
				*pDest++ = SChar<T>::ToLower(*pSrc++);
			}

			// Return new string
			return CCoreString<T>(pNew);
		}

		CCoreString<T> SubString(int iStart, int iLength = -1)
		{
			int thisLength = GetLength();

			if (iStart < 0)
				iStart = thisLength + iStart;

			if (iLength < 0)
				iLength = thisLength - iStart;

			assert(iStart >= 0);
			assert(iStart + iLength <= thisLength);

			return CCoreString<T>(m_pData->m_sz + iStart, iLength);
		}

		template <typename S = SCase>
		int IndexOf(const T* find, int startOffset = 0) const
		{
			// Check bounds
			if (find == nullptr || *find == 0 || m_pData == nullptr || m_pData->m_iLength == 0)
				return -1;

			// Get search string length
			int srcLen = SChar<T>::Length(find);

			// Find it
			int stopPos = m_pData->m_iLen - srcLen;
			for (int i = startOffset; i <= stopPos; i++)
			{
				if (S::Compare(m_pData->m_sz + i, find, srcLen) == 0)
					return i;
			}

			return -1;
		}

		template <class S = SCase>
		int IndexOfAny(const T* chars, int startOffset = 0) const
		{
			if (m_pData == nullptr)
				return -1;

			for (int i=startOffset; i<m_pData->m_iLength; i++)
			{
				if (IsOneOf<S>(chars, m_pData->m_sz[i]))
					return i;
			}

			return -1;
		}

		template <typename S>
		static bool IsOneOf(const T* chars, T ch)
		{
			while (*chars)
			{
				if (S::Compare(*chars, ch) == 0)
					return true;
				chars++;
			}
			return false;
		}

		template <class S = SCase>
		int LastIndexOf(const T* find, int startOffset = -1) const
		{
			// Check bounds
			if (find == nullptr || *find == 0 || m_pData == nullptr || m_pData->m_iLength == 0)
				return -1;

			// Get search string length
			int srcLen = SChar<T>::Length(find);

			if (startOffset < 0)
				startOffset = m_pData->m_iLength - 1;

			// Find it
			for (int i = startOffset; i >= 0; i--)
			{
				if (S::Compare(m_pData->m_sz + i, find, srcLen) == 0)
					return i;
			}

			return -1;
		}

		template <class S = SCase>
		int LastIndexOfAny(const T* chars, int startOffset = -1) const
		{
			if (m_pData == nullptr)
				return -1;

			if (startOffset < 0)
				startOffset = m_pData->m_iLength - 1;

			for (int i=startOffset; i>=0; i--)
			{
				if (IsOneOf<S>(chars, m_pData->m_sz[i]))
				{
					return i;
				}
			}

			return -1;
		}

		template <class S = SCase>
		CCoreString<T> Replace(const T* find, const T* replace, int maxReplacements = -1, int startOffset = 0)
		{
			// Start offset past end of string?
			assert(startOffset <= GetLength());

			// Setup builder and copy the bit before start index
			CCoreStringBuilder<T> builder;
			if (startOffset > 0)
				builder.Append(sz(), startOffset);

			// Replace string
			builder.template ReplaceAppend<S>(sz() + startOffset, find, replace, maxReplacements);

			// Store in self
			return builder.ToString();
		}


		static bool IsNullOrEmpty(const T* a)
		{
			return a == nullptr || *a == '\0';
		}

		template <class S = SCase>
		bool IsEqualTo(const T* b) const
		{
			return S::AreEqual(sz(), b);
		}

		template <class S = SCase>
		bool StartsWith(const T* find) const
		{
			if (m_pData == nullptr)
				return false;
			return S::Compare(m_pData->m_sz, find, SChar<T>::Length(find)) == 0;
		}

		template <class S = SCase>
		bool EndsWith(const T* find) const
		{
			if (m_pData == nullptr)
				return false;
			int findLen = SChar<T>::Length(find);
			int startPos = m_pData->m_iLength  - findLen;
			if (startPos < 0)
				return false;
			return S::Compare(m_pData->m_sz + startPos, find, findLen) == 0;
		}

		static CCoreString<T> Join(CVector<CCoreString<T>>& parts, T separator)
		{
			CCoreStringBuilder<T> sb;
			for (int i=0; i<parts.GetCount(); i++)
			{
				if (i > 0)
					sb.Append(separator);
				sb.Append(parts[i]);
			}
			return sb.ToString();
		}

		template <typename S = SCase>
		int Split(const T* separators, bool includeEmpty, CVector<CCoreString<T>>& parts) const
		{
			// Clear buffer
			parts.Clear();

			if (m_pData == nullptr)
				return 0;

			// Get start of string
			const T* p = m_pData->m_sz;
			if (!p)
				return 0;

			// Split
			const T* pPart = p;
			while (*p)
			{
				if (IsOneOf<S>(separators, *p))
				{
					if (includeEmpty || p > pPart)
						parts.Add(CCoreString<T>(pPart, (int)(p - pPart)));

					pPart = p + 1;
					p = pPart;
				}
				else
				{
					p++;
				}
			}

			if (includeEmpty || p > pPart)
				parts.Add(CCoreString<T>(pPart, (int)(p - pPart)));

			return parts.GetCount();
		}


		static CCoreString<T> Format(const T* pFormat, ...)
		{
			va_list args;
			va_start(args, pFormat);
			CCoreString<T> result = FormatV(pFormat, args);
			va_end(args);
			return result;
		}

		static CCoreString<T> FormatV(const T* pFormat, va_list args)
		{
			CCoreStringBuilder<T> buf;
			buf.FormatV(pFormat, args);
			return buf.ToString();
		}


		bool CopyToBuffer(T* buf, int buflenChars)
		{
			// Check will fit
			int srclen = GetLength();
			if (srclen + 1 > buflenChars)
				return false;

			// Copy it
			if (srclen)
				memcpy(buf, m_pData->m_sz, srclen * sizeof(T));
			buf[srclen] = '\0';
			return true;
		}

		T* AllocCopy(int withLengthInChars=0)
		{
			if (m_pData == nullptr)
			{
				return nullptr;
			}
			else
			{
				int length = GetLength() + 1;
				if (withLengthInChars != 0 && withLengthInChars < length)
					return nullptr;
				T* dest = (T*)malloc(length * sizeof(T));
				memcpy(dest, m_pData->m_sz, length * sizeof(T));
				return dest;
			}
		}

	protected:
		struct StringData
		{
			int m_iRef;
			int m_iLength;
			T	m_sz[1];
		};

		static StringData* AllocCoreStringData(const T* psz, int length)
		{
			if (psz == nullptr && length <= 0)
				return nullptr;
			if (length < 0)
				length = SChar<T>::Length(psz);
			StringData* p = (StringData*)malloc(sizeof(StringData) + length * sizeof(T));
			p->m_iRef = 1;
			p->m_iLength = length;
			if (psz)
				memcpy(p->m_sz, psz, length * sizeof(T));
			p->m_sz[length] = '\0';
			return p;
		}

		StringData* m_pData;

	private:
		CCoreString(StringData* pData)
		{
			m_pData = pData;
		}
	};

	typedef CCoreString<char> CString;

}	// namespace

#endif  // __simplelib_string_h__