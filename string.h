#ifndef __simplelib_string_h__
#define __simplelib_string_h__

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "semantics.h"
#include "formatting.h"

namespace SimpleLib
{
	/*

	Simple string class stores a string.

	Implements "copy on write" for effecient copy of CString to CString
		(eg: function return values etc...)

	Also, implemented with internal data prefixed to string memory so string class can be
		passed as a string pointer for sprintf type functions. (ie: the whole class is same
		size as a pointer)  (See nested CHeader struct + Get/SetHeader functions)

	*/

	template <typename T=char>
	class CString
	{
	public:
		// Constructor
		CString()
		{
			SetHeader(nullptr);
		}

		// Constructor
		CString(const CString<T>& Other)
		{
			m_psz = Other.m_psz;
			if (m_psz)
			{
				GetHeader()->m_iRef++;
			}
		}

		// Constructor
		CString(const T* psz, int iLen = -1)
		{
			SetHeader(nullptr);
			Assign(psz, iLen);
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
			assert(m_psz);
			assert(iPos >= 0 && iPos < GetHeader()->m_iMemSize);
			if (GetHeader()->m_iRef > 1)
			{
				GetBuffer(-1);
			}
			return m_psz[iPos];
		}

		// const T* operator
		operator const T* () const
		{
			return m_psz;
		}

		// Get null terminated string
		const T* sz() const
		{
			return m_psz;
		}

		// Free extra space
		void FreeExtra() 
		{
			// Get header, quit if none
			CHeader* pHeader = GetHeader();
			if (!pHeader)
				return;

			// Work out length if invalid
			if (pHeader->m_iLength < 0)
				pHeader->m_iLength = len(m_psz);

			// Get new buffer if shared...
			if (pHeader->m_iRef > 1)
				GetBuffer(pHeader->m_iLength + 1);

			// If using excessive memory, shrink...
			if (pHeader->m_iLength + 16 < pHeader->m_iMemSize)
			{
				// Work out new length
				int iNewBufSize = pHeader->m_iLength + 1;

				// Reallocate
				pHeader = (CHeader*)realloc(pHeader, sizeof(CHeader) + sizeof(T) * iNewBufSize);
				if (!pHeader)
					return;

				// Store new size...
				pHeader->m_iMemSize = iNewBufSize;

				// Store new mem pointer
				SetHeader(pHeader);
			}
		}

		void Clear()
		{
			if (!m_psz)
				return;

			if (GetHeader()->m_iRef > 1)
			{
				GetHeader()->m_iRef--;
			}
			else
			{
				free(GetHeader());
			}
			m_psz = nullptr;
		}

		bool IsEmpty() const
		{
			return GetLength() == 0;
		}

		bool Assign(const CString<T>& Other)
		{
			Clear();
			m_psz = Other.m_psz;
			if (m_psz)
				GetHeader()->m_iRef++;
			return true;
		}

		bool Assign(const T* psz, int iLen = -1)
		{
			// Clear old value
			Clear();

			// Quit if null string
			if (!psz)
				return true;

			// Auto length?
			if (iLen < 0)
				iLen = SChar<T>::Length(psz);

			if (!GetBuffer(iLen))
				return false;

			// Copy new value in
			memcpy(m_psz, psz, iLen * sizeof(T));
			m_psz[iLen] = L'\0';

			// Store new text size
			GetHeader()->m_iLength = iLen;

			return true;
		}

		int GetLength() const
		{
			CHeader* pHeader = GetHeader();
			if (!pHeader)
				return 0;
			if (pHeader->m_iLength < 0)
				pHeader->m_iLength = SChar<T>::Length(m_psz);
			return pHeader->m_iLength;
		}

		bool ReplaceRange(int iPos, int iOldLen, const T* psz, int iNewLen = -1)
		{
			// Quit if nothing to do
			if (!iOldLen && !iNewLen)
				return true;

			// Auto length?
			if (iNewLen < 0)
			{
				iNewLen = psz ? SChar<T>::Length(psz) : 0;
			}

			// Append?
			if (iPos < 0)
				iPos = GetLength();

			// Check position in range
			assert(iPos <= GetLength());

			// Replace entire RHS?
			if (iOldLen < 0)
				iOldLen = GetLength() - iPos;

			// Check in range
			assert(iPos + iOldLen <= GetLength());

			// Work out new required size
			int iNewTextSize = GetLength() + iNewLen - iOldLen;
			assert(iNewTextSize >= 0);

			int iLength = GetLength();

			// Reallocate buffer
			if (!GrowBuffer(iNewTextSize))
				return false;

			// Move trailing characters
			int iTrailingChars = iLength - (iPos + iOldLen);
			if (iTrailingChars && iOldLen != iNewLen)
			{
				memmove(m_psz + iPos + iNewLen, m_psz + iPos + iOldLen, iTrailingChars * sizeof(T));
			}

			// Copy new characters
			if (iNewLen)
			{
				memcpy(m_psz + iPos, psz, iNewLen * sizeof(T));
			}

			// Update size/position
			m_psz[iNewTextSize] = L'\0';
			GetHeader()->m_iLength = iNewTextSize;
			return true;
		}

		bool Append(const T* psz, int iLen = -1)
		{
			return ReplaceRange(GetLength(), 0, psz, iLen);
		}


		bool Append(const T ch)
		{
			return Append(&ch, 1);
		}


		bool InsertAt(int iPos, const T* psz, int iLen = -1)
		{
			return ReplaceRange(iPos, 0, psz, iLen);
		}


		bool DeleteRange(int iPos, int iLen = -1)
		{
			return ReplaceRange(iPos, iLen, nullptr, 0);
		}


		CString<T>& operator+=(const T* psz)
		{
			Append(psz);
			return *this;
		}

		CString<T>& operator+=(T ch)
		{
			Append(ch);
			return *this;
		}

		static CString<T> SubString(const T* psz, int iLen, int iStart, int iLength)
		{
			if (!psz)
				return nullptr;

			if (iLen < 0)
				iLen = SChar<T>::Length(psz);

			if (iStart > iLen)
				return nullptr;

			if (iStart < 0)
				iStart = iLen + iStart;

			if (iLength < 0)
				iLength = iLen - iStart;

			if (iStart + iLength > iLen)
				iLength = iLen - iStart;

			return CString<T>(psz + iStart, iLength);
		}


		CString<T> SubString(int iStart, int iCount = -1)
		{
			return SubString(sz(), GetLength(), iStart, iCount);
		}

		template <class SCase = SCaseSensitive<T>>
		int IndexOf(const T* psz, int startOffset = 0) const
		{
			return IndexOf(m_psz, psz, startOffset);
		}

		template <class SCase = SCaseSensitive<T>>
		static int IndexOf(const T* psz, const T* find, int startOffset = 0)
		{
			if (find == nullptr)
				return -1;

			// Get search string length
			int srcLen = SChar<T>::Length(find);
			if (srcLen == 0)
				return startOffset;

			int destLen = SChar<T>::Length(psz);
			if (destLen == 0)
				return -1;

			// Find it
			int stopPos = destLen - srcLen;
			for (int i = startOffset; i <= stopPos; i++)
			{
				if (SCase::Compare(psz + i, find, srcLen) == 0)
					return i;
			}

			return -1;
		}


		template <class SCase = SCaseSensitive<T>>
		void Replace(const T* find, const T* replace, int maxReplacements = -1, int startOffset = 0)
		{
			int findLen = SChar<T>::Length(find);
			int replaceLen = SChar<T>::Length(replace);

			CString<T> strNew = *this;

			while (true)
			{
				// Find it
				int foundPos = strNew.IndexOf<SCase>(find, startOffset);
				if (foundPos < 0)
					break;

				// Replace it
				strNew.ReplaceRange(foundPos, findLen, replace, replaceLen);

				// Continue searching after
				startOffset = foundPos + replaceLen;

				// Limit replacements
				maxReplacements--;
				if (maxReplacements == 0)
					break;
			}

			Assign(strNew);
		}


		static bool IsNullOrEmpty(const T* a)
		{
			return a == nullptr || *a == '\0';
		}

		template <class SCase = SCaseSensitive<T>>
		bool IsEqualTo(const T* b) const
		{
			return IsEqual(m_psz, b);
		}

		template <class SCase = SCaseSensitive<T>>
		static bool IsEqual(const T* a, const T* b)
		{
			return Compare<SCase>(a, b) == 0;
		}

		template <class SCase = SCaseSensitive<T>>
		static bool Compare(const T* a, const T* b)
		{
			if (a == nullptr) a = SChar<T>::EmptyString();
			if (b == nullptr) b = SChar<T>::EmptyString();
			return SCase::Compare(a, b);
		}

		template <class SCase = SCaseSensitive<T>>
		bool StartsWith(const T* find) const
		{
			if (m_psz == nullptr)
				return false;
			return SCase::Compare(m_psz, find, SChar<T>::Length(find)) == 0;
		}

		template <class SCase = SCaseSensitive<T>>
		static bool StartsWith(const T* psz, const T* find)
		{
			if (psz == nullptr)
				return false;
			return SCase::Compare(psz, find, SChar<T>::Length(find)) == 0;
		}

		template <class SCase = SCaseSensitive<T>>
		bool EndsWith(const T* find) const
		{
			int findLen = SChar<T>::Length(find);
			int startPos = GetLength() - findLen;
			if (startPos < 0)
				return false;
			return SCase::Compare(m_psz + startPos, find, findLen) == 0;
		}

		template <class SCase = SCaseSensitive<T>>
		static bool EndsWith(const T* psz, const T* find)
		{
			if (psz == nullptr)
				return false;
			int findLen = SChar<T>::Length(find);
			int startPos = SChar<T>::Length(psz) - findLen;
			if (startPos < 0)
				return false;
			return SCase::Compare(psz + startPos, find, findLen) == 0;
		}

		int Split(ICharSet<T>* separator, bool includeEmpty, CVector<CString<T>>& parts) const
		{
			return Split(m_psz, separator, includeEmpty, parts);
		}

		static int Split(const T* psz, const ICharSet<T>& separator, bool includeEmpty, CVector<CString<T>>& parts)
		{
			// Clear buffer
			parts.RemoveAll();

			// Get start of string
			const T* p = psz;
			if (!p)
				return 0;

			// Split
			const T* pPart = p;
			while (*p)
			{
				if (separator.IsChar(*p))
				{
					if (includeEmpty || p > pPart)
						parts.Add(CString<T>(pPart, p - pPart));

					pPart = p + 1;
					p = pPart;
				}
				else
				{
					p++;
				}
			}

			if (includeEmpty || p > pPart)
				parts.Add(CString<T>(pPart, p - pPart));

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
			CFormatOutputMemory<T> helper;
			CFormatting::FormatV(&helper, pFormat, args);
			return (const T*)helper;
		}


		// Ensures at least iBufSize, grows to iBufSize
		T* GetBuffer(int iBufSize = -1)
		{
			CHeader* pHeader = GetHeader();

			if (iBufSize < 0)
			{
				iBufSize = pHeader->m_iLength;
				if (iBufSize < 0)
					iBufSize = GetLength() + 1;
			}

			// Copy on write...
			if (pHeader && pHeader->m_iRef > 1)
			{

				// Allocate new header
				pHeader = (CHeader*)malloc(sizeof(CHeader) + sizeof(T) * iBufSize);
				if (!pHeader)
					return nullptr;

				// Copy from original string
				memcpy(pHeader, GetHeader(), sizeof(CHeader) + sizeof(T) * Min(GetHeader()->m_iMemSize, iBufSize));

				// Release original string
				GetHeader()->m_iRef--;

				// Setup new header
				pHeader->m_iMemSize = iBufSize;
				pHeader->m_iRef = 1;
				pHeader->m_iLength = -1;
				SetHeader(pHeader);

				// Done
				return m_psz;
			}

			// Check if need to resize
			if (pHeader && iBufSize <= pHeader->m_iMemSize)
			{
				pHeader->m_iLength = -1;
				return m_psz;
			}

			// Alloc/Grow buffer...
			if (pHeader)
			{
				CHeader* pNewHeader = (CHeader*)realloc(pHeader, sizeof(CHeader) + sizeof(T) * iBufSize);
				if (!pNewHeader)
					return nullptr;
				pHeader = pNewHeader;
			}
			else
			{
				pHeader = (CHeader*)malloc(sizeof(CHeader) + sizeof(T) * iBufSize);
				if (!pHeader)
					return nullptr;
			}

			// Store new buffer
			SetHeader(pHeader);
			pHeader->m_iMemSize = iBufSize;
			pHeader->m_sz[iBufSize] = 0;
			pHeader->m_iRef = 1;

			// Invalidate length
			pHeader->m_iLength = -1;

			// Done!
			return m_psz;
		}

		// Smart grow for appending, doubles buffer size when too small
		T* GrowBuffer(int iNewSize)
		{
			// If no buffer allocate at requested size
			if (!m_psz)
				return GetBuffer(iNewSize);

			// Copy on write?
			if (GetHeader()->m_iRef > 1)
				return GetBuffer(iNewSize);

			// Quit if already big enough
			if (iNewSize <= GetHeader()->m_iMemSize)
				return m_psz;

			// Instead of just growing by a little bit, double the buffer size
			// (to save lots of tiny reallocs when appending)
			int iDoubleSize = GetHeader()->m_iMemSize * 2;
			if (iDoubleSize > iNewSize)
				iNewSize = iDoubleSize;

			// Resize the buffer, but maintain the current length
			int iLen = GetHeader()->m_iLength;
			GetBuffer(iNewSize);
			GetHeader()->m_iLength = iLen;

			return m_psz;
		}

	protected:
		struct CHeader
		{
			int m_iRef;
			int	m_iMemSize;
			int m_iLength;
			T	m_sz[1];
		};

		// Return the outer class from the address of a member variable
		#define outerclassptr(outerClassName, memberName, ptrToMember) \
			reinterpret_cast<outerClassName*>(reinterpret_cast<char*>(ptrToMember) - offsetof(outerClassName, memberName))

		CHeader* GetHeader() const { return m_psz ? outerclassptr(CHeader, m_sz, m_psz) : nullptr; }
		void SetHeader(CHeader* pHeader) { m_psz = pHeader ? pHeader->m_sz : nullptr; }

		T* m_psz;
	};

}	// namespace

#endif  // __simplelib_string_h__