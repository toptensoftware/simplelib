#pragma once

#include <assert.h>

namespace SimpleLib
{
	template <typename T>
	class OwnedPtr
	{
	public:
		// Constructor
		OwnedPtr(T* ptr = nullptr)
		{
			_ptr = ptr;
		}

		// Destructor
		~OwnedPtr()
		{
			if (_ptr)
				delete _ptr;
		}


		// Copy (not allowed)
		OwnedPtr(const OwnedPtr&) = delete;
		OwnedPtr& operator=(const OwnedPtr&) = delete;		

		// Move
		OwnedPtr(OwnedPtr&& other)
		{
			_ptr = other._ptr;
			other._ptr = nullptr;
		}
		OwnedPtr& operator=(OwnedPtr&& other)
		{
			_ptr = other._ptr;
			other._ptr = nullptr;
		}

		// Assign to	
		T* operator=(T* pNew)
		{
			if (_ptr)
				delete _ptr;	
			_ptr = pNew;
		}

		// Detach
		T* Detach()
		{
			T* val = _ptr;
			_ptr = nullptr;
			return val;
		}

		operator T*() const
		{
			return _ptr;
		}

		T* operator->() const
		{
			assert(_ptr != nullptr);
			return _ptr;
		}

		T& operator*() const
		{
			assert(_ptr != nullptr);
			return *_ptr;
		}

		operator bool() const
		{
			return _ptr != nullptr;
		}

		using TSemantics = SValue<T*>; 

	private:
		T* _ptr	;
	};
}
