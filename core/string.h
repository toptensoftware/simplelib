#pragma once

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>

#include "List.h"
#include "StringBuilder.h"


namespace SimpleLib
{
	/*
	Simple immutable string class stores a string.
	*/

	template <typename T>
	class StringCore
	{
	public:
		// Constructor
		StringCore()
		{
			m_pData = nullptr;
		}

		// Copy Constructor
		StringCore(const StringCore<T>& other)
		{
			m_pData = nullptr;
			Assign(other);
		}

		// Move Constructor
		StringCore(StringCore<T>&& other)
		{
			m_pData = other.m_pData;
			other.m_pData = nullptr;
		}

		// Constructor
		StringCore(const T* psz, int iLength)
		{
			m_pData = AllocStringData(psz, iLength);
		}

		// Constructor
		StringCore(const T* psz)
		{
			if (psz == nullptr)
				m_pData = nullptr;
			else
				m_pData = AllocStringData(psz, -1);
		}

		// Constructor
		StringCore(const StringBuilder<T>& builder)
		{
			int length;
			const T* psz = builder.Finish(&length);
			m_pData = AllocStringData(psz, length);
		}

		// Destructor
		~StringCore()
		{
			Clear();
		}

		// Types
		typedef StringCore<T> _CString;

		// Assignment
		StringCore<T>& operator=(const StringCore<T>& Other)
		{
			Assign(Other);
			return *this;
		}

		// Move Assignment
		StringCore<T>& operator=(StringCore<T>&& Other)
		{
			m_pData = Other.m_pData;
			Other.m_pData = nullptr;
			return *this;
		}

		// Assignment
		StringCore<T>& operator=(const T* psz) 
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

		bool operator ==(const StringCore<T>& b) const
		{
			return this->IsEqualTo(b.sz());
		}

		bool operator !=(const StringCore<T>& b) const
		{
			return !this->IsEqualTo(b.sz());
		}

		bool operator <(const StringCore<T>& b) const
		{
			return SCase::Compare(sz(), b.sz()) < 0;
		}

		bool operator >(const StringCore<T>& b) const
		{
			return SCase::Compare(sz(), b.sz()) > 0;
		}

		bool operator <=(const StringCore<T>& b) const
		{
			return SCase::Compare(sz(), b.sz()) <= 0;
		}

		bool operator >=(const StringCore<T>& b) const
		{
			return SCase::Compare(sz(), b.sz()) >= 0;
		}

		// True if the string is null or empty
		bool operator !() const
		{
			return IsNullOrEmpty(sz());
		}

		StringCore<T> operator+(const StringCore<T>& other) const
		{
			StringBuilder<T> builder;
			builder.Append(sz(), GetLength());
			builder.Append(other.sz(), other.GetLength());
			return builder;
		}

		StringCore<T> operator+=(const T* psz)
		{
			StringBuilder<T> builder;
			builder.Append(sz(), GetLength());
			builder.Append(psz);
			*this = StringCore<T>(builder);
			return *this;
		}

		StringCore<T> operator+=(const StringCore<T>& other)
		{
			*this = *this + other;
			return *this;
		}

/*
		StringCore& operator+=(const T* psz)
		{
			*this = *this + psz;
			return *this;
		}
*/


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

		bool IsNull() const
		{
			return m_pData == nullptr;
		}

		bool IsEmpty() const
		{
			return GetLength() == 0;
		}

		bool Assign(const StringCore<T>& Other)
		{
			Clear();
			m_pData = Other.m_pData;
			if (m_pData)
				m_pData->m_iRef++;
			return true;
		}

		void Assign(const T* psz, int iLength = -1)
		{
			// Clear old value
			Clear();

			// Store new
			m_pData = AllocStringData(psz, iLength);
		}

		int GetLength() const
		{
			return m_pData ? m_pData->m_iLength : 0;
		}

		StringCore<T> ToUpper()
		{
			if (m_pData == nullptr)
				return StringCore<T>();

			// Allocate new string buffer
			StringData* pNew = AllocStringData(nullptr, GetLength());

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
			return StringCore<T>::WithData(pNew);
		}

		StringCore<T> ToLower()
		{
			if (m_pData == nullptr)
				return StringCore<T>();

			// Allocate new string buffer
			StringData* pNew = AllocStringData(nullptr, GetLength());

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
			return StringCore<T>::WithData(pNew);
		}

		StringCore<T> SubString(int iStart, int iLength = -1)
		{
			int thisLength = GetLength();

			if (iStart < 0)
				iStart = thisLength + iStart;

			if (iLength < 0)
				iLength = thisLength - iStart;

			assert(iStart >= 0);
			assert(iStart + iLength <= thisLength);

			return StringCore<T>(m_pData->m_sz + iStart, iLength);
		}

		template <typename S = SCase>
		int IndexOf(T find, int startOffset = 0) const
		{
			// Check bounds
			if (m_pData == nullptr || m_pData->m_iLength == 0)
				return -1;

			// Find it
			for (int i = startOffset; i <= m_pData->m_iLength; i++)
			{
				if (S::Compare(m_pData->m_sz[i], find) == 0)
					return i;
			}

			return -1;
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
			int stopPos = m_pData->m_iLength - srcLen;
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
		StringCore<T> Replace(const T* find, const T* replace, int maxReplacements = -1, int startOffset = 0)
		{
			// Start offset past end of string?
			assert(startOffset <= GetLength());

			// Setup builder and copy the bit before start index
			StringBuilder<T> builder;
			if (startOffset > 0)
				builder.Append(sz(), startOffset);

			// Replace string
			builder.template ReplaceAppend<S>(sz() + startOffset, find, replace, maxReplacements);

			// Store in self
			return builder.Finish();
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

		static StringCore<T> Join(List<StringCore<T>>& parts, T separator)
		{
			StringBuilder<T> sb;
			for (int i=0; i<parts.GetCount(); i++)
			{
				if (i > 0)
					sb.Append(separator);
				sb.Append(parts[i]);
			}
			return sb.Finish();
		}

		template <typename S = SCase>
		int Split(const T* separators, bool includeEmpty, List<StringCore<T>>& parts) const
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
						parts.Add(StringCore<T>(pPart, (int)(p - pPart)));

					pPart = p + 1;
					p = pPart;
				}
				else
				{
					p++;
				}
			}

			if (includeEmpty || p > pPart)
				parts.Add(StringCore<T>(pPart, (int)(p - pPart)));

			return parts.GetCount();
		}

		static StringCore<T> Format(const T* pFormat, ...)
		{
			va_list args;
			va_start(args, pFormat);
			StringCore<T> result = FormatV(pFormat, args);
			va_end(args);
			return result;
		}

		static StringCore<T> FormatV(const T* pFormat, va_list args)
		{
			StringBuilder<T> buf;
			buf.FormatV(pFormat, args);
			return buf.Finish();
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

	static uint32_t Hash(const StringCore& str)
	{
		return hash_buf(str.sz(), str.GetLength() * sizeof(T));
	}

	protected:
		struct StringData
		{
			int m_iRef;
			int m_iLength;
			T	m_sz[1];
		};

		static StringData* AllocStringData(const T* psz, int length)
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

		static StringCore<T> WithData(StringData* pData)
		{
			StringCore<T> s;
			s.m_pData = pData;
			return s;
		}

		StringData* m_pData;
	};

	typedef StringCore<char> String;
	typedef StringCore<wchar_t> WString;
	typedef StringCore<char32_t> String32;
}
