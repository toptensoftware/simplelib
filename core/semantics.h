#pragma once

namespace SimpleLib
{

// Value semantics
template <typename T>
class SValue
{
public:
	typedef T TArg;
	typedef T TStorage;

	static TArg Detach(TStorage& storage) { return storage; }
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

