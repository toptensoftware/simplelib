#pragma once

#include "MpmcStack.h"
#include "HighWaterHeap.h"

namespace SimpleLib
{

// Implements a lock free high-water heap set.
// This is designed for a very specific use case - fast, lock free, but short-lived
// memory allocations.
class HighWaterHeapSet
{
public:
    HighWaterHeapSet(int initialBuckets,  uint32_t bucketSize)
    {
        // Store bucket size
        m_bucketSize = bucketSize;

        // Allocate the initial buckets
        for (int i=0; i<initialBuckets; i++)
        {
            m_reserveBuckets.Push(new Bucket(bucketSize));
        }
    }

    ~HighWaterHeapSet()
    {
        DeleteAll(m_activeBuckets);
        DeleteAll(m_reserveBuckets);
    }

    // Allocate memory
    void* Alloc(size_t size)
    {
        // The idea here is we want to continue to use the same active buckets repeatedly until
        // they fill up.   Once full a bucket will be drained out to empty as the allocations
        // are freed (remember this is designed for short lived allocations) and eventually
        // returned to the reserve list
        Bucket* triedBuckets = nullptr;
        Bucket* bucket;
        while (bucket = m_activeBuckets.Pop())
        {
            // If it's empty, retire it to the reserve pool and move on to the
            // next active bucket - once pushed, another thread may pop and
            // start using it immediately, so we must not keep using or
            // re-linking it ourselves (Bucket has a single intrusive `next`
            // field shared by both stacks - it can only belong to one at a
            // time).
            // Note: GetLikelyUsed() check for zero is not a guess - no one else has a reference
            // to this bucket so nothing can be allocated from it.  And if its already empty,
            // nothing else could since be freed from it.
            if (bucket->GetLikelyUsed() == 0)
            {
                m_reserveBuckets.Push(bucket);
                continue;
            }

            // Try the allocation in this bucket
            void* mem = bucket->Alloc(size);
            if (mem)
            {
                // Put the succeeding bucket and any other tried buckets back on the active list
                // (with the succeeding one on top)
                bucket->next = triedBuckets;
                triedBuckets = bucket;
                m_activeBuckets.PushMany(bucket);
                return mem;
            }

            // Save a list of buckets we've tried
            bucket->next = triedBuckets;
            triedBuckets = bucket;
        }

        // Put the tried buckets back on the active list
        if (triedBuckets)
            m_activeBuckets.PushMany(triedBuckets);

        // Try the reserve list
        bucket = m_reserveBuckets.Pop();
        if (!bucket)
            return nullptr;
        void* mem = bucket->Alloc(size);
        m_activeBuckets.Push(bucket);

        // Done
        return mem;
    }

    // Free memory
    void Free(void* mem)
    {
        // Pass through...
        HighWaterHeap::FreeFromCorrectHeap(mem);
    }

protected:

    class Bucket : public HighWaterHeap
    {
    public:
        Bucket(uint32_t capacity) : HighWaterHeap(capacity)
        {
        }

        Bucket* next = nullptr;       // For MpmcStack
    };

    // Pop and delete every bucket remaining on a stack
    static void DeleteAll(MpmcStack<Bucket>& stack)
    {
        Bucket* bucket = stack.PopAll();
        while (bucket)
        {
            Bucket* next = bucket->next;
            delete bucket;
            bucket = next;
        }
    }

    uint32_t m_bucketSize;
    MpmcStack<Bucket> m_reserveBuckets;
    MpmcStack<Bucket> m_activeBuckets;
};


}
