#ifndef __simplelib_sharedptr_h__
#define __simplelib_sharedptr_h__

#include <assert.h>

namespace SimpleLib
{

template <typename T>
class CSharedPtr
{
public:
	CSharedPtr(T* ptr = nullptr)
	{
		if (ptr == nullptr)
			_pControl = nullptr;
		else
			_pControl = new CONTROL(ptr);
	}

	CSharedPtr(const CSharedPtr& other)
	{
		_pControl = other._pControl;
		if (_pControl)
			_pControl->AddRef();
	}

	CSharedPtr(CSharedPtr&& other)
	{
		_pControl = other._pControl;
		other._pControl = nullptr;
	}

	~CSharedPtr()
	{
		if (_pControl)
			_pControl->Release();
	}

	T* operator->() const
	{
		assert(_pControl != nullptr);
		return _pControl->_ptr;
	}

	bool operator!() const
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

	const CSharedPtr<T>& operator=(const CSharedPtr<T>& other)
	{
		if (_pControl)
			_pControl->Release();

		_pControl = other._pControl;

		if (_pControl)
			_pControl->AddRef();
		return *this;
	}

	CSharedPtr<T>& operator=(CSharedPtr<T>&& Other)
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


}	// namespace

#endif // __simplelib_sharedptr_h__