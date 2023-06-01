#pragma once

#ifdef _MSC_VER

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
	inline void* __CRTDECL operator new(size_t, void* _Where)
	{
		return (_Where);
	}
#if _MSC_VER >= 1200
	inline void __CRTDECL operator delete(void*, void*)
	{
		return;
	}
#endif
#endif
#endif

#ifdef __GNUC__

	// Placed new/delete operators
	inline void* operator new(size_t, void* pMem)
	{
		return pMem;
	}
	inline void operator delete(void* pMem, void*)
	{
	}

#endif

namespace SimpleLib
{
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

} // namespace
