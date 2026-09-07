#pragma once

#include <stdlib.h>
#include <string.h>

#include "Formatting.h"
#include "StringSemantics.h"

namespace SimpleLib
{
	template <typename T>
	class StringCore;

	// Simple StringBuilder class that uses embedded short buffer but switches
	// to dynamic allocations for longer strings
	template <typename T>
	class StringBuilder : public IStringWriter<T>
	{
	public:
		// Constructor
		StringBuilder()
		{
			m_iLength = 0;
			m_pMem = m_shortBuffer;
			m_iCapacity = sizeof(m_shortBuffer) / sizeof(T);
		}

		// Destructor
		virtual ~StringBuilder()
		{
			Reset();
		}

		// Reset the stringbuilder, releasing any dynamically allocated buffer
		void Reset()
		{
			if (m_pMem != m_shortBuffer)
				free(m_pMem);
			m_iLength = 0;
			m_pMem = m_shortBuffer;
			m_iCapacity = sizeof(m_shortBuffer) / sizeof(T);
		}

		// Clear, keeping memory buffer
		void Clear()
		{
			m_iLength = 0;
		}

		int GetLength()
		{
			return m_iLength;
		}

		void Truncate(int newLength)
		{
			assert(newLength >= 0);
			assert(newLength <= m_iLength);
			m_iLength = newLength;
		}

		// Rescans the content for a NUL terminator and sets the builder's
		// internally tracked length to match (eg: after writing through a
		// buffer obtained via GetBuffer() using nul-terminated C APIs).
		// Returns *this so it can be chained eg: sb.SyncLength().ToString()
		StringBuilder& SyncLength()
		{
			int length = SChar<T>::Length(m_pMem);
			assert(length <= m_iCapacity);
			m_iLength = length;
			return *this;
		}

		// IStringWriter
		virtual void Write(T ch) override
		{
			Append(ch);
		}

		// Append a character
		void Append(T ch)
		{
			*Reserve(1) = ch;
		}

		// Append a character multiple times
		void Append(T ch, int count)
		{
			T* p = Reserve(count);
			for (int i=0; i<count; i++)
			{
				*p++ = ch;
			}
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

		StringBuilder& operator += (T ch)
		{
			Append(&ch, 1);
			return *this;
		}

		StringBuilder& operator += (const T* psz)
		{
			Append(psz);
			return *this;
		}

		// Make sure the buffer is big enough for a specifed number
		// of characters and return pointer to uninitialized buffer
		// NB: This reserves space at the end of anything already
		//     allocated
		T* Reserve(int length)
		{
			if (m_iLength + length > m_iCapacity)
			{
				// Work out new capacity
				int newCapacity = m_iCapacity;
				while (newCapacity < m_iLength + length)
					newCapacity *= 2;

				if (m_pMem == m_shortBuffer)
				{
					T* pNew = (T*)malloc(sizeof(T) * newCapacity);
					if (pNew == nullptr)
						return nullptr;
					m_pMem = pNew;
					memcpy(m_pMem, m_shortBuffer, m_iLength * sizeof(T));
				}
				else
				{
					T* pNew = (T*)realloc(m_pMem, sizeof(T) * newCapacity);
					if (pNew == nullptr)
						return nullptr;
					m_pMem = pNew;
				}
			}

			// Take room
			T* retv = m_pMem + m_iLength;
			m_iLength += length;
			return retv;
		}

		// Like Reserve(), but first clears the builder back to empty so the
		// returned buffer starts at offset zero
		T* GetBuffer(int length)
		{
			Clear();
			return Reserve(length);
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
			if (m_iLength == 0 || (m_iLength > 0 && m_pMem[m_iLength - 1] != '\0'))
			{
				const_cast<StringBuilder*>(this)->Append((T)'\0');
			}
			*piLength = m_iLength - 1;
			return m_pMem;
		}

		// Return the builder's content as a StringCore
		StringCore<T> ToString() const
		{
			return StringCore<T>(*this);
		}

		T* Detach()
		{
			// Make sure null terminated
			int length;
			Finish(&length);

			// Can't just detach short buffer, so alloc
			if (m_pMem == m_shortBuffer)
			{
				T* retv = (T*)malloc(length + 1);
				memcpy(retv, m_pMem, length + 1);
				m_iLength = 0;
				return retv;
			}
			else
			{
				T* retv = m_pMem;
				Reset();
				return retv;
			}
		}

		operator const T* () const
		{
			return Finish();
		}

		const T* sz() const
		{
			return Finish();
		}

		template <class S = SCase>
		int ReplaceAppend(const T* input, const T* find, const T* replace, int maxReplacements = -1)
		{
			int findLen = SChar<T>::Length(find);
			if (findLen == 0)
			{
				Append(input);
				return 0;
			}

			const T* p = input;
			int count = 0;
			while (*p)
			{
				if (S::Compare(p, find, findLen) == 0)
				{
					Append(replace);
					p += findLen;
					count++;
					if (count == maxReplacements)
					{
						Append(p);
						return count;
					}
				}
				else
				{
					Append(*p);
					p++;
				}
			}

			return count;
		}

		static void write_callback(void* arg, T ch)
		{
			((StringBuilder*)arg)->Append(ch);
		}

		void FormatV(const T* pFormat, va_list args)
		{
			FormatStringWriter<T> fw(this);
			Formatting::FormatV(&fw, pFormat, args);
		}

		void Format(const T* pFormat, ...)
		{
			va_list args;
			va_start(args, pFormat);
			FormatV(pFormat, args);
			va_end(args);
		}


	private:
		T* m_pMem;
		int m_iLength;
		int m_iCapacity;
		T m_shortBuffer[128];
	};

}