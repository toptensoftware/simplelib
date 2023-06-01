#pragma once

#include <stdlib.h>
#include <string.h>

#include "string_semantics.h"

namespace SimpleLib
{
	// Manages a pool of temporary strings that can all be freed at once
	template <typename T>
	class StringPool
	{
	public:
		// Constructor
		StringPool(int defaultBucketSize = 4096)
		{
			m_pFirstBucket = nullptr;
			m_iDefaultBucketSize = defaultBucketSize;
		}

		// Destructor
		virtual ~StringPool()
		{
			Reset();
		}

		// Reset the pool, deleting all allocated memory
		void Reset()
		{
			BUCKET* p = m_pFirstBucket;
			while (p)
			{
				BUCKET* pNext = p->next;
				free(p);
				p = pNext;
			}
			m_pFirstBucket = nullptr;
		}

		// Free all strings, but keep allocated memory
		void FreeAll()
		{
			BUCKET* p = m_pFirstBucket;
			while (p)
			{
				p->used = 0;
				p = p->next;
			}
		}

		// Allocate a copy of a string
		T* Alloc(const T* psz)
		{
			if (psz == nullptr)
				return nullptr;
			return Alloc(psz, SChar<T>::Length(psz));
		}

		// Allocate a copy of string with specified length
		T* Alloc(const T* psz, int length)
		{
			T* p = Reserve(length + 1);
			if (psz)
				memcpy(p, psz, length * sizeof(T));
			p[length] = '\0';
			return p;
		}

		// Reserve room in the string pool for uninitialized string
		T* Reserve(int length)
		{
			// Find a bucket
			BUCKET* p = FindBucketWithRoom(length);

			// Calculate address
			T* psz = ((T*)(p + 1)) + p->used;

			// Claim room
			p->used += length;

			// Done
			return psz;
		}

	private:
		struct BUCKET
		{
			BUCKET* next;
			int		used;
			int		capacity;
		};

		// Find a bucket with specified amount of room
		BUCKET* FindBucketWithRoom(int length)
		{
			// Find a bucket where it fits
			BUCKET* p = m_pFirstBucket;
			BUCKET* pPrev = nullptr;
			BUCKET* pEmptyBucket = nullptr;
			BUCKET* pEmptyBucketPrev = nullptr;
			while (p)
			{
				if (p->used + length < p->capacity)
				{
					break;
				}
				if (p->used == 0)
				{
					pEmptyBucket = p;
					pEmptyBucketPrev = pPrev;
				}
				pPrev = p;
				p = p->next;
			}

			if (p == nullptr)
			{
				if (pEmptyBucket != nullptr)
				{
					int newBucketSize = pEmptyBucket->capacity * sizeof(T) + sizeof(BUCKET);
					while (newBucketSize - sizeof(BUCKET) < length * sizeof(T))
						newBucketSize = newBucketSize * 2;
					BUCKET* pNew = (BUCKET*)realloc(pEmptyBucket, newBucketSize);
					if (pNew)
					{
						pNew->capacity = (newBucketSize - sizeof(BUCKET)) / sizeof(T);
						if (pEmptyBucketPrev)
							pEmptyBucketPrev->next = pNew;
						else
							m_pFirstBucket = pNew;
						p = pNew;
					}
				}

				if (p == nullptr)
				{
					p = AllocateBucket(length);
				}
			}

			return p;
		}

		// Allocate a new bucket
		BUCKET* AllocateBucket(int length)
		{
			int newBucketSize = m_iDefaultBucketSize;
			while (newBucketSize - sizeof(BUCKET) < length * sizeof(T))
				newBucketSize = newBucketSize * 2;

			BUCKET* p = (BUCKET*)malloc(newBucketSize);
			p->next = m_pFirstBucket;
			p->used = 0;
			p->capacity = (newBucketSize - sizeof(BUCKET)) / sizeof(T);
			m_pFirstBucket = p;

			return p;
		}

		// The first bucket in the chain
		BUCKET* m_pFirstBucket;
		int m_iDefaultBucketSize;
	};

}