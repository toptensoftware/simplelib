#pragma once

namespace SimpleLib
{
	template <typename T>
	class Buffer
	{
	public:
		Buffer(int initialSize = 0)
		{
			_pData = nullptr;
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
			if (_size != size)
			{
				free(_pData);
				_pData = malloc(size * sizeof(T));
				_size = size;
			}
		}

		void EnsureSize(int size)
		{
			if (_size < size)
				SetCapacity(_size);
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
