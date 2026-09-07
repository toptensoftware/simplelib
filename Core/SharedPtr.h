#pragma once

#include <assert.h>

namespace SimpleLib
{
	template <typename T>
	class SharedPtr
	{
	public:
		SharedPtr(T* ptr = nullptr)
		{
			if (ptr == nullptr)
				_pControl = nullptr;
			else
				_pControl = new CONTROL(ptr);
		}

		SharedPtr(const SharedPtr& other)
		{
			_pControl = other._pControl;
			if (_pControl)
				_pControl->AddRef();
		}

		SharedPtr(SharedPtr&& other)
		{
			_pControl = other._pControl;
			other._pControl = nullptr;
		}

		~SharedPtr()
		{
			if (_pControl)
				_pControl->Release();
		}

		operator T*() const
		{
			assert(_pControl != nullptr);
			return _pControl->_ptr;
		}

		T* operator->() const
		{
			assert(_pControl != nullptr);
			return _pControl->_ptr;
		}

		T& operator*() const
		{
			assert(_pControl != nullptr);
			return *_pControl->_ptr;
		}

		operator bool() const
		{
			return _pControl != nullptr;
		}

		T* operator=(T* pNew)
		{
			if (_pControl)
				_pControl->Release();
			if (pNew)
				_pControl = new CONTROL(pNew);
			else
				_pControl = nullptr;
			return pNew;
		}

		bool operator==(const T* other) const
		{
			return static_cast<const void*>(_pControl ? _pControl->_ptr : nullptr) == 
					static_cast<const void*>(other);
		}

		bool operator!=(const T* other) const 
		{
			return !(*this == other);
		}

		bool operator==(const SharedPtr<T>& other) const
		{
			return static_cast<const void*>(_pControl ? _pControl->_ptr : nullptr) == 
					static_cast<const void*>(other._pControl ? other._pControl->_ptr : nullptr);
		}

		bool operator!=(const SharedPtr<T>& other) const 
		{
			return !(*this == other);
		}

		const SharedPtr<T>& operator=(const SharedPtr<T>& other)
		{
			if (_pControl)
				_pControl->Release();

			_pControl = other._pControl;

			if (_pControl)
				_pControl->AddRef();
			return *this;
		}

		SharedPtr<T>& operator=(SharedPtr<T>&& Other)
		{
			_pControl = Other._pControl;
			Other._pControl = nullptr;
		}
		
	private:
		struct CONTROL
		{
			int _nRef;
			T* _ptr;

			CONTROL(T* ptr)
			{
				_ptr = ptr;
				_nRef = 1;
			}

			~CONTROL()
			{
				if (_ptr)
					delete _ptr;
			}

			void AddRef()
			{
				_nRef++;
			}

			void Release()
			{
				_nRef--;
				if (_nRef == 0)
					delete this;
			}
		};

		CONTROL* _pControl;
	};
}
