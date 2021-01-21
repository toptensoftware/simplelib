#ifndef __simplelib_semantics_wchar_h__
#define __simplelib_semantics_wchar_h__

#include "semantics.h"

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
	};


} // namespace

#endif  // __simplelib_semantics_wchar_h__