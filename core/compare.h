#pragma once

#include <type_traits>
#include "HashUtils.h"
#include "StringSemantics.h"

namespace SimpleLib
{

	template <typename T>
	class StringCore;

	// Detect if T has a static Hash(const T&) member
	template <typename T, typename = void>
	struct has_static_hash : std::false_type {};

	template <typename T>
	struct has_static_hash<T, std::void_t<decltype(T::Hash(std::declval<const T&>()))>>
		: std::true_type {};
		
	// Default comparator
	struct SDefaultCompare
	{
		template <typename T>
		static int Compare(const T& a, const T& b)
		{
			return a > b ? 1 : a < b ? -1 : 0;
		}

		// Overload for StringCore<T> - compares string contents directly
		// via SCase rather than the generic operator</> (which would
		// otherwise need to be evaluated twice for a single comparison)
		template <typename T>
		static int Compare(const StringCore<T>& a, const StringCore<T>& b)
		{
			return SCase::Compare(a.sz(), b.sz());
		}

		template <typename T>
		static bool AreEqual(const T& a, const T& b)
		{
			return a == b;
		}


		template <typename T>
		static uint32_t Hash(const T& a)
		{
			if constexpr (has_static_hash<T>::value)
			{
				return T::Hash(a);
			}
			else
			{
				static_assert(sizeof(T) == 0, "No default Hash implementation for this type; specialize Hash for T");
				return 0;
			}
		}

		static uint32_t Hash(int32_t a)  { return hash32(a); }
		static uint32_t Hash(uint32_t a) { return hash32(a); }
		static uint32_t Hash(int64_t a)  { return hash32_64(a); }
		static uint32_t Hash(uint64_t a) { return hash32_64(a); }

		template <typename T>
		static uint32_t Hash(T* a)   // catches all pointer types via a separate template overload
		{
			return Hash((size_t)a);
		}
	};

}

