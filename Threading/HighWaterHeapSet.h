#pragma once

#include "MpmcStack.h"
#include "HighWaterHeap.h"

namespace SimpleLib
{

class HighWaterHeapSet
{
protected:
    class Bucket : public HighWaterHeap
    {
    public:
        Bucket(uint32_t capacity) : HighWaterHeap(capacity)
        {
        }

        Bucket* next;       // For MpmcStack
    };

    public:
    HighWaterHeapSet(int initialBuckets,  uint32_t bucketSize)
    {
        m_bucketSize = bucketSize;
        for (int i=0; i<initialBuckets; i++)
        {
            m_reserveBuckets.Push(new Bucket(bucketSize));
        }
    }

    ~HighWaterHeapSet()
    {

    }

    // Allocate memory 
    void* Alloc(size_t size)
    {
        Bucket* triedBuckets = nullptr;
        Bucket* lastTriedBucket = nullptr;
        Bucket* bucket;
        while (bucket = m_activeBuckets.Pop())
        {
            void* mem = bucket->Alloc(size);
            if (mem)
            {
                m_activeBuckets.Push(triedBuckets, lastTriedBucket);
                return mem;
            }

            // Save in list
            if (lastTriedBucket == nullptr)
                lastTriedBucket = bucket;
            bucket->next = triedBuckets;
            triedBuckets = bucket;
        }
    }

    // Free memory
    void Free(void* mem)
    {
        // Free from allocated heap
        HighWaterHeap::FreeFromCorrectHeap(mem);
    }

protected:
    class Bucket : public HighWaterHeap
    {
    public:
        Bucket(uint32_t capacity) : HighWaterHeap(capacity)
        {
        }

        Bucket* next;       // For MpmcStack
    };

    uint32_t m_bucketSize;
    MpmcStack<Bucket> m_reserveBuckets;
    MpmcStack<Bucket> m_activeBuckets;
};


}