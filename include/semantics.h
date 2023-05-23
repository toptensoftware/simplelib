#ifndef __simplelib_semantics_h__
#define __simplelib_semantics_h__

#include <ctype.h>
#include <stdarg.h>
#include <wchar.h>
#include <string.h>

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

	// Storage semantics for simple values
	struct SStorageValue
	{
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
	};

	// Storage semantics for owned pointers
	struct SStorageOwnedPtr
	{
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
		{ 

		}
	};

	// Compare semantics for simple values
	struct SCompareValue
	{
		template <typename T>
		static int Compare(const T& a, const T& b)
		{
			return a > b ? 1 : a < b ? -1 : 0;
		}

		template <typename T>
		static bool AreEqual(const T& a, const T& b)
		{
			return a == b;
		}
	};

	// Semantics for simple values
	struct SValue
	{
		typedef SStorageValue TStorage;
		typedef SCompareValue TCompare;
	};

	// Semantics for owned pointers
	struct SOwnedPtr
	{
		typedef SStorageOwnedPtr TStorage;
		typedef SCompareValue TCompare;
	};

	// Produces the default semantics for a type
	// the default being SValue semantics
	template <typename T>
	struct SDefaultSemantics
	{
		typedef SValue TSemantics;
	};

	// Produces the type of argument to use for stored
	// values of a specified type
	template <typename T>
	struct SArgType
	{
		typedef const T& TArgType;
	};



} // namespace

#endif  // __simplelib_semantics_h__


/*
	template<typename T, T val>
	struct value_constant { static const T value = val; };
	struct value_true : value_constant<bool, true> {};
	struct value_false : value_constant<bool, false> {};

	template <typename A, typename = void> struct has_lt : value_false {};
	template <typename A> struct has_lt<A, decltype((void)&A::operator <)> : value_true {};

	template <typename T> struct is_integral : value_false {};
	template <>	struct is_integral<char> : value_true {};
	template <>	struct is_integral<unsigned char> : value_true {};
	template <>	struct is_integral<short> : value_true {};
	template <>	struct is_integral<unsigned short> : value_true {};
	template <>	struct is_integral<int> : value_true {};
	template <>	struct is_integral<unsigned int> : value_true {};
	template <>	struct is_integral<long> : value_true {};
	template <>	struct is_integral<unsigned long> : value_true {};
	template <>	struct is_integral<long long> : value_true {};
	template <>	struct is_integral<unsigned long long> : value_true {};
	template <typename T> struct is_floatingpoint : value_false {};
	template <>	struct is_floatingpoint<float> : value_true {};
	template <>	struct is_floatingpoint<double> : value_true {};
	template <>	struct is_floatingpoint<long double> : value_true {};
	template <typename T> struct is_numeric : value_constant<bool, 
		is_integral<T>::value || 
		is_floatingpoint<T>::value> {};
	template<typename T> struct is_reference : value_false {};
	template<typename T> struct is_reference<T&> : value_true {};

	template <typename T>
	class SCompare
	{
	public:
		static int Compare(T a, T b)
		{			
			if constexpr (has_lt<T>::value || is_numeric<T>::value)
			{
				return (b < a) ? (1) : (a < b ? -1 : 0);
			}
			else
			{
				assert(false && "Type doesn't support comparison");
				return 0;
			}
		}
	};
*/

