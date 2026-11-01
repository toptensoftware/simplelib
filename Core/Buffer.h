#pragma once

#include <stdlib.h>

namespace SimpleLib
{
	// Simple uninitialized memory buffer for POD types. Resizing preserves
	// existing contents (up to the new size).
	template <typename T>
	class Buffer
	{
	public:
		Buffer(int initialSize = 0)
		{
			_pData = nullptr;
			_size = 0;
			SetCapacity(initialSize);
		}

		~Buffer()
		{
			free(_pData);
		}

		operator T* () const
		{
			return (T*)_pData;
		}

		void SetCapacity(int size)
		{
			if (_size == size)
				return;

			if (size <= 0)
			{
				free(_pData);
				_pData = nullptr;
				_size = 0;
				return;
			}

			void* pNew = realloc(_pData, size * sizeof(T));
			if (pNew == nullptr)
				return;		// Allocation failed, keep existing buffer
			_pData = pNew;
			_size = size;
		}

		// Grow the buffer to at least `size` elements (never shrinks)
		void EnsureSize(int size)
		{
			if (_size < size)
				SetCapacity(size);
		}

		int GetCapacity() const
		{
			return _size;
		}

	private:
		void* _pData;
		int _size;
		Buffer(const Buffer& other);
		Buffer& operator=(const Buffer& other);
	};
}
