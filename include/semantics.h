#pragma once

namespace SimpleLib
{

	// Value semantics
	template <typename T>
	class SValue
	{
	public:
		typedef T TArg;
		static void Retain(const TArg& val)
		{
		}
		static void Release(const TArg& val)
		{
		}
	};

	// Owned pointer semantics
	template <typename T>
	class SOwnedPtr
	{
	public:
		typedef SOwnedPtr<T> TSemantics;

		typedef T* TArg;

		static void Retain(const TArg& val)
		{

		}
		static void Release(const TArg& val)
		{
			delete val;
		}
	};


	template<typename... Ts>
	using if_type_exists = void;

	template <typename T, typename = void>
	struct get_semantics {
		using TSemantics = SValue<T>; 
	};

	template <typename T>
	struct get_semantics<T, if_type_exists<typename T::TSemantics>> {
		using TSemantics = typename T::TSemantics;
	};


}

