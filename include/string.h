#ifndef __simplelib_string_h__
#define __simplelib_string_h__

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>

#include "semantics.h"
#include "vector.h"
#include "stringbuilder.h"

namespace SimpleLib
{
	/*
	Simple immutable string class stores a string.
	*/

	template <typename T=char>
	class CString
	{
	public:
		// Constructor
		CString()
		{
			m_pData = nullptr;
		}

		// Constructor
		CString(const CString<T>& Other)
		{
			m_pData = nullptr;
			Assign(Other);
		}

		// Constructor
		CString(const T* psz, int iLen = -1)
		{
			m_pData = AllocStringData(psz, iLen);
		}

		// Constructor
		CString(const CStringBuilder<T>& builder)
		{
			int length;
			const T* psz = builder.ToString(&length);
			m_pData = AllocStringData(psz, length);
		}

		// Destructor
		~CString()
		{
			Clear();
		}

		// Types
		typedef CString<T> _CString;

		// Assignment
		CString<T>& operator=(const CString<T>& Other)
		{
			Assign(Other);
			return *this;
		}

		// Assignment
		CString<T>& operator=(const T* psz) 
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

		bool operator ==(const CString<T>& b) const
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

		bool Assign(const CString<T>& Other)
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
			m_pData = AllocStringData(psz, iLen);
		}

		int GetLength() const
		{
			return m_pData ? m_pData->m_iLength : 0;
		}

		CString<T> ToUpper()
		{
			if (m_pData == nullptr)
				return CString<T>();

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
			return CString<T>(pNew);
		}

		CString<T> ToLower()
		{
			if (m_pData == nullptr)
				return CString<T>();

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
			return CString<T>(pNew);
		}

		CString<T> SubString(int iStart, int iLength = -1)
		{
			int thisLength = GetLength();

			if (iStart < 0)
				iStart = thisLength + iStart;

			if (iLength < 0)
				iLength = thisLength - iStart;

			assert(iStart >= 0);
			assert(iStart + iLength <= thisLength);

			return CString<T>(m_pData->m_sz + iStart, iLength);
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
		CString<T> Replace(const T* find, const T* replace, int maxReplacements = -1, int startOffset = 0)
		{
			// Start offset past end of string?
			assert(startOffset <= GetLength());

			// Setup builder and copy the bit before start index
			CStringBuilder<T> builder;
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

		static CString<T> Join(CVector<CString<T>>& parts, T separator)
		{
			CStringBuilder<T> sb;
			for (int i=0; i<parts.GetCount(); i++)
			{
				if (i > 0)
					sb.Append(separator);
				sb.Append(parts[i]);
			}
			return sb.ToString();
		}

		template <typename S = SCase>
		int Split(const T* separators, bool includeEmpty, CVector<CString<T>>& parts) const
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
						parts.Add(CString<T>(pPart, (int)(p - pPart)));

					pPart = p + 1;
					p = pPart;
				}
				else
				{
					p++;
				}
			}

			if (includeEmpty || p > pPart)
				parts.Add(CString<T>(pPart, (int)(p - pPart)));

			return parts.GetCount();
		}


		static CString<T> Format(const T* pFormat, ...)
		{
			va_list args;
			va_start(args, pFormat);
			CString<T> result = FormatV(pFormat, args);
			va_end(args);
			return result;
		}

		static CString<T> FormatV(const T* pFormat, va_list args)
		{
			CStringBuilder<T> buf;
			buf.FormatV(pFormat, args);
			return buf.sz();
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

		StringData* m_pData;

	private:
		CString(StringData* pData)
		{
			m_pData = pData;
		}
	};

	// String semantics
	template <typename T>
	struct SString
	{
		typedef SStorageValue TStorage;
		typedef SCase TCompare;
	};

	// Make SString the default semantics for CString
	template <typename T>
	struct SDefaultSemantics<CString<T>>
	{
		typedef SString<T> TSemantics;
	};

	// Make collections of CString<T> accept 'const T*' 
	// as arguments instead of const CString<T>& which
	// would require memory allocation to pass.
	template <typename T>
	struct SArgType<CString<T>>
	{
		typedef const T* TArgType;
	};

}	// namespace

#endif  // __simplelib_string_h__