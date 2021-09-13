#ifndef __simplelib_semantics_wchar_h__
#define __simplelib_semantics_wchar_h__

#include "semantics.h"
#include "string.h"

namespace SimpleLib
{
	template <>
	class SChar<wchar_t>
	{
	public:
		static int Length(const wchar_t* a)
		{
#ifdef _MSC_VER
			return (int)wcslen(a);
#else
			const wchar_t* p = a;
			while (*p) p++;
			return (int)(p - a);
#endif
		}

		static const wchar_t* EmptyString() 
		{ 
			return L""; 
		}

		static bool IsEmpty(const wchar_t* a)
		{
			return a == nullptr || wcslen(a) == 0;
		}

	};

	template <>
	class SCaseSensitive<wchar_t>
	{
	public:
		static bool IsEqual(const wchar_t* a, const wchar_t* b)
		{
			if (a == nullptr && b == nullptr)
				return true;
			if (a == nullptr || b == nullptr)
				return false;
			return wcscmp(a, b) == 0;
		}

		static int Compare(wchar_t a, wchar_t b)
		{
			return b - a;
		}

		static int Compare(const wchar_t* a, const wchar_t* b)
		{
			return wcscmp(a, b);
		}

		static int Compare(const wchar_t* a, const wchar_t* b, int length)
		{
			return wcsncmp(a, b, length);
		}
	};

	template <>
	inline int Compare<CString<wchar_t>>(const CString<wchar_t>& a, const CString<wchar_t>& b)
	{
		return SCaseSensitive<wchar_t>::Compare(a, b);
	}


} // namespace

#endif  // __simplelib_semantics_wchar_h__