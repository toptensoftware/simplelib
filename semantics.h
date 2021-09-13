#ifndef __simplelib_semantics_h__
#define __simplelib_semantics_h__

#include <ctype.h>
#include <stdarg.h>

#ifdef _MSC_VER
#define DEPRECATED(x) __declspec(deprecated(x))

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
	inline void* __CRTDECL operator new(size_t, void* _Where)
	{
		return (_Where);
	}
#if     _MSC_VER >= 1200
	inline void __CRTDECL operator delete(void*, void*)
	{
		return;
	}
#endif
#endif
#endif

#ifdef __GNUC__

#define DEPRECATED(x) __attribute__ ((deprecated(x)))

	// Placed new/delete operators
	inline void* operator new(size_t, void* pMem)
	{
		return pMem;
	}
	inline void operator delete(void* pMem, void*)
	{
	}

#include <strings.h>

#endif

namespace SimpleLib
{
	// Minimum
	template <typename T>
	T Min(const T& a, const T& b) { return a < b ? a : b; }

	// Maximum
	template <typename T>
	T Max(const T& a, const T& b) { return a > b ? a : b; }

	// Generic compare function
	template <typename T>
	int Compare(const T& a, const T& b)
	{
		return a > b ? 1 : a < b ? -1 : 0;
	}

	// Placed construction
	template<typename T, typename T2> inline
	void Constructor(T* ptr, const T2& src)
	{
		new ((void*)ptr) T(src);
	}

	// Placed destruction
	template <typename T> inline
	void Destructor(T* ptr)
	{
		ptr->~T();
	}

	// Simple value semantics
	class SValue
	{
	public:
		template <typename T, typename TOwner>
		static const T& OnAdd(const T& val, TOwner* pOwner)
		{
			return val;
		}

		template <typename T, typename TOwner>
		static void OnRemove(T& val, TOwner* pOwner)
		{ }

		template <typename T, typename TOwner>
		static void OnDetach(T& val, TOwner* pOwner)
		{ }

		template <typename T1, typename T2>
		static int Compare(const T1& a, const T2& b)
		{
			return SimpleLib::Compare(a, b);
		}

	};

	// Owned ptr semantics
	class SOwnedPtr
	{
	public:
		template <typename T, typename TOwner>
		static const T& OnAdd(const T& val, TOwner* pOwner)
		{
			return val;
		}

		template <typename T, typename TOwner>
		static void OnRemove(T& val, TOwner* pOwner)
		{
			delete val;
		}

		template <typename T, typename TOwner>
		static void OnDetach(T& val, TOwner* pOwner)
		{ }

		template <typename T>
		static int Compare(const T& a, const T& b)
		{
			return SimpleLib::Compare(a, b);
		}
	};

	template <typename T>
	struct ICharSet
	{
		virtual bool IsChar(T ch) const = 0;
	};

	template <typename T>
	struct CCharSetSingle : public ICharSet<T>
	{
	public:
		CCharSetSingle(T ch)
		{
			_ch = ch;
		}
		virtual bool IsChar(T ch) const override
		{
			return ch == _ch;
		}
		T _ch;
	};

	// Character semantics
	template <typename T>
	class SChar
	{
	};

	// Formatter - either include simplelib/formatting.h in your project or,
	// declare your own implementation making sure it's in the SimpleLib namespace
	// eg:
	// namespace SimpleLib
	// {
	// 	template <typename T>
	// 	void vcbprintf(void (*write)(void*, T), void* arg, const T* format, va_list args)
	// 	{
	// 		// Your formatter impl
	// 	}
	// }
	template <typename T>
	void simplelib_vcbprintf(void (*write)(void*, T), void* arg, const T* format, va_list args);

	template <>
	class SChar<char>
	{
	public:
		static int Length(const char* a)
		{
			return (int)strlen(a);
		}

		static const char* EmptyString() 
		{ 
			return ""; 
		}

		static bool IsEmpty(const char* a)
		{
			return a == nullptr || strlen(a) == 0;
		}

		static bool IsEqual(const char* a, const char* b)
		{
			if (a == nullptr && b == nullptr)
				return true;
			if (a != nullptr || b != nullptr)
				return false;
			return strcmp(a, b) == 0;
		}
	};

	template <typename T>
	class SCaseSensitive
	{

	};

	template <typename T>
	class SCaseInsensitive
	{

	};

	template <>
	class SCaseSensitive<char>
	{
	public:
		static int Compare(char a, char b)
		{
			return b - a;
		}

		static int Compare(const char* a, const char* b, int len)
		{
			return strncmp(a, b, len);
		}

		static int Compare(const char* a, const char* b)
		{
			return strcmp(a, b);
		}

	};

	template <>
	class SCaseInsensitive<char>
	{
	public:
		static int Compare(char a, char b)
		{
			return toupper(b) - toupper(a);
		}

		static int Compare(const char* a, const char* b, int len)
		{
#ifdef _MSC_VER
			return _strnicmp(a, b, len);
#else
			return strncasecmp(a, b, len);
#endif
		}

		static int Compare(const char* a, const char* b)
		{
#ifdef _MSC_VER
			return _stricmp(a, b);
#else
			return strcasecmp(a, b);
#endif
		}
	};



} // namespace

#endif  // __simplelib_semantics_h__