#ifndef __simplelib_string_semantics_h__
#define __simplelib_string_semantics_h__

#include <ctype.h>
#include <stdarg.h>
#include <wchar.h>
#include <string.h>


namespace SimpleLib
{

	template <typename T>
	struct IStringWriter
	{
		virtual void Write(T ch) = 0;
	};

	// Character semantics
	template <typename T>
	struct SChar
	{
		static int Length(const T* a)
		{
			const T* p = a;
			while (*p) p++;
			return (int)(p - a);
		}

		static bool IsEmpty(const wchar_t* a)
		{
			return a == nullptr || *a == 0;
		}
	};

	template <>
	struct SChar<char>
	{
		static char ToUpper(char ch) { return toupper(ch); }
		static char ToLower(char ch) { return tolower(ch); }

		static int Length(const char* a)
		{
			return (int)strlen(a);
		}

		static const char* EmptyString() 
		{ 
			return ""; 
		}
	};

	template <>
	class SChar<wchar_t>
	{
	public:
#ifdef _MSC_VER
		static char ToUpper(char ch) { return towupper(ch); }
		static char ToLower(char ch) { return towlower(ch); }
#else
		static char ToUpper(char ch) { return toupper(ch); }
		static char ToLower(char ch) { return tolower(ch); }
#endif

		static int Length(const wchar_t* a)
		{
			return (int)wcslen(a);
		}

		static const wchar_t* EmptyString() 
		{ 
			return L""; 
		}
	};



	class SCase
	{
	public:
		// char

		static int Compare(char a, char b)
		{
			return b - a;
		}

		static int Compare(const char* a, const char* b)
		{
			if (a == nullptr && b == nullptr)
				return 0;
			if (a == nullptr)
				return 1;
			if (b == nullptr)
				return -1;
			return strcmp(a, b);
		}

		static int Compare(const char* a, const char* b, int len)
		{
			if (a == nullptr && b == nullptr)
				return 0;
			if (a == nullptr)
				return 1;
			if (b == nullptr)
				return -1;
			return strncmp(a, b, len);
		}

		static bool AreEqual(const char* a, const char* b)
		{
			return Compare(a, b) == 0;
		}

		// char16_t

		static int Compare(char16_t a, char16_t b)
		{
			return b - a;
		}

		static int Compare(const char16_t* a, const char16_t* b)
		{
			if (a == nullptr && b == nullptr)
				return 0;
			if (a == nullptr)
				return 1;
			if (b == nullptr)
				return -1;
			while (*a && *b && *a == *b)
			{
				a++;
				b++;
			}
			return *a - *b;
		}

		static int Compare(const char16_t* a, const char16_t* b, int len)
		{
			if (a == nullptr && b == nullptr)
				return 0;
			if (a == nullptr)
				return 1;
			if (b == nullptr)
				return -1;
			const char16_t* end = a + len;
			while (a < end)
			{
				if (*a != *b)
				{
					return *a - *b;
				}
				a++;
				b++;
			}
			return 0;
		}

		static bool AreEqual(const char16_t* a, const char16_t* b)
		{
			return Compare(a, b) == 0;
		}

		// wchar_t

		static int Compare(wchar_t a, wchar_t b)
		{
			return b - a;
		}

		static int Compare(const wchar_t* a, const wchar_t* b)
		{
			if (a == nullptr && b == nullptr)
				return 0;
			if (a == nullptr)
				return 1;
			if (b == nullptr)
				return -1;
			return wcscmp(a, b);
		}

		static int Compare(const wchar_t* a, const wchar_t* b, int len)
		{
			if (a == nullptr && b == nullptr)
				return 0;
			if (a == nullptr)
				return 1;
			if (b == nullptr)
				return -1;
			return wcsncmp(a, b, len);
		}

		static bool AreEqual(const wchar_t* a, const wchar_t* b)
		{
			return Compare(a, b) == 0;
		}

		// char32_t

		static int Compare(char32_t a, char32_t b)
		{
			return b - a;
		}

		static int Compare(const char32_t* a, const char32_t* b)
		{
			if (a == nullptr && b == nullptr)
				return 0;
			if (a == nullptr)
				return 1;
			if (b == nullptr)
				return -1;
			while (*a && *b && *a == *b)
			{
				a++;
				b++;
			}
			return *a - *b;
		}

		static int Compare(const char32_t* a, const char32_t* b, int len)
		{
			if (a == nullptr && b == nullptr)
				return 0;
			if (a == nullptr)
				return 1;
			if (b == nullptr)
				return -1;
			const char32_t* end = a + len;
			while (a < end)
			{
				if (*a != *b)
				{
					return *a - *b;
				}
				a++;
				b++;
			}
			return 0;
		}

		static bool AreEqual(const char32_t* a, const char32_t* b)
		{
			return Compare(a, b) == 0;
		}

	};

	class SCaseI
	{
	public:
		static int Compare(char a, char b)
		{
			return toupper(b) - toupper(a);
		}

		static int Compare(const char* a, const char* b)
		{
			if (a == nullptr && b == nullptr)
				return 0;
			if (a == nullptr)
				return 1;
			if (b == nullptr)
				return -1;
#ifdef _MSC_VER
			return stricmp(a, b);
#else
			return strcasecmp(a, b);
#endif
		}

		static int Compare(const char* a, const char* b, int len)
		{
			if (a == nullptr && b == nullptr)
				return 0;
			if (a == nullptr)
				return 1;
			if (b == nullptr)
				return -1;
#ifdef _MSC_VER
			return strnicmp(a, b, len);
#else
			return strncasecmp(a, b, len);
#endif
		}

		static bool AreEqual(const char* a, const char* b)
		{
			return Compare(a, b) == 0;
		}

		static int Compare(wchar_t a, wchar_t b)
		{
			return SChar<wchar_t>::ToUpper(b) - SChar<wchar_t>::ToUpper(a);
		}

		static int Compare(const wchar_t* a, const wchar_t* b)
		{
			if (a == nullptr && b == nullptr)
				return 0;
			if (a == nullptr)
				return 1;
			if (b == nullptr)
				return -1;
#ifdef _MSC_VER
			return wcsicmp(a, b);
#else
			while (*a && *b)
			{
				int compare = Compare(*a++, *b++);
				if (compare != 0)
					return compare;
			}
			return Compare(*a, *b);
#endif
		}

		static int Compare(const wchar_t* a, const wchar_t* b, int len)
		{
			if (a == nullptr && b == nullptr)
				return 0;
			if (a == nullptr)
				return 1;
			if (b == nullptr)
				return -1;
#ifdef _MSC_VER
			return wcsnicmp(a, b, len);
#else
			const wchar_t* end = a + len;
			while (a < end)
			{
				int compare = Compare(*a++, *b++);
				if (compare != 0)
					return compare;
				a++;
				b++;
			}
			return 0;
#endif
		}

		static bool AreEqual(const wchar_t* a, const wchar_t* b)
		{
			return Compare(a, b) == 0;
		}
	};


} // namespace

#endif  // __simplelib_string_semantics_h__

