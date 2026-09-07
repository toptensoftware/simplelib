#pragma once

#include <assert.h>

namespace SimpleLib
{
	// Smart pointer for objects that manage their own reference count via
	// AddRef()/Release() methods (eg: COM objects implementing IUnknown).
	//
	// Like a raw interface pointer returned from CoCreateInstance/QueryInterface,
	// constructing (or assigning) from a raw T* adopts an existing reference
	// without calling AddRef - the caller is expected to already own that
	// reference. Copying a RefCountedPtr calls AddRef to create a new one.
	template <typename T>
	class RefCountedPtr
	{
	public:
		// Constructor - adopts an existing reference (does not AddRef)
		RefCountedPtr(T* ptr = nullptr)
		{
			_ptr = ptr;
		}

		// Copy
		RefCountedPtr(const RefCountedPtr& other)
		{
			_ptr = other._ptr;
			if (_ptr)
				_ptr->AddRef();
		}

		// Move
		RefCountedPtr(RefCountedPtr&& other)
		{
			_ptr = other._ptr;
			other._ptr = nullptr;
		}

		// Destructor
		~RefCountedPtr()
		{
			if (_ptr)
				_ptr->Release();
		}

		// Copy assignment
		RefCountedPtr& operator=(const RefCountedPtr& other)
		{
			if (other._ptr)
				other._ptr->AddRef();
			if (_ptr)
				_ptr->Release();
			_ptr = other._ptr;
			return *this;
		}

		// Move assignment
		RefCountedPtr& operator=(RefCountedPtr&& other)
		{
			if (_ptr != other._ptr && _ptr)
				_ptr->Release();
			_ptr = other._ptr;
			other._ptr = nullptr;
			return *this;
		}

		// Assign from raw pointer - adopts an existing reference (does not AddRef)
		T* operator=(T* pNew)
		{
			if (pNew != _ptr && _ptr)
				_ptr->Release();
			_ptr = pNew;
			return _ptr;
		}

		// Detach - releases ownership of the reference without calling Release
		T* Detach()
		{
			T* val = _ptr;
			_ptr = nullptr;
			return val;
		}

		// Address-of - for passing to out-params (eg: CoCreateInstance, QueryInterface).
		// Asserts the pointer is currently null so an existing reference isn't leaked.
		T** operator&()
		{
			assert(_ptr == nullptr);
			return &_ptr;
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

	private:
		T* _ptr;
	};
}
