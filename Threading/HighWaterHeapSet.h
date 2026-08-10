#pragma once

#include "MpmcStack.h"
#include "HighWaterHeap.h"

#include <atomic>
#include <thread>
#include <cstdio>

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

    // ---- lock-free diagnostic ring buffer (temporary) ----
    struct LogEntry
    {
        unsigned tid;
        const char* tag;
        void* bucket;
        void* nextVal;
    };
    static const int kLogSize = 65536;
    inline static LogEntry s_log[kLogSize] = {};
    inline static std::atomic<unsigned> s_logIdx{0};
    inline static std::atomic<bool> s_dumped{false};

    static unsigned Tid()
    {
        static std::atomic<unsigned> s_next{1};
        thread_local unsigned id = s_next.fetch_add(1);
        return id;
    }

    static void DLog(const char* tag, void* bucket, void* nextVal)
    {
        unsigned idx = s_logIdx.fetch_add(1, std::memory_order_relaxed) % kLogSize;
        s_log[idx] = { Tid(), tag, bucket, nextVal };
    }

    // Dump every log entry that mentions `filterBucket` (its full history),
    // plus the full unfiltered tail for local context. Only the first
    // caller gets to dump (others just return) so we get one clean report.
    static void DumpLog(const char* why, void* filterBucket)
    {
        bool expected = false;
        if (!s_dumped.compare_exchange_strong(expected, true))
            return;

        unsigned end = s_logIdx.load();
        unsigned totalLogged = end;
        unsigned start = (end > kLogSize) ? end - kLogSize : 0;

        fprintf(stderr, "\n==== HighWaterHeapSet diagnostic dump (%s) filterBucket=%p totalLogged=%u ====\n", why, filterBucket, totalLogged);
        fprintf(stderr, "---- full history for bucket %p ----\n", filterBucket);
        for (unsigned i = start; i < end; i++)
        {
            LogEntry& e = s_log[i % kLogSize];
            if (e.bucket == filterBucket)
                fprintf(stderr, "#%u [tid%u] %-28s bucket=%p next=%p\n", i, e.tid, e.tag, e.bucket, e.nextVal);
        }
        fprintf(stderr, "---- unfiltered tail (last 200) ----\n");
        unsigned tailStart = (end > 200) ? end - 200 : 0;
        for (unsigned i = tailStart; i < end; i++)
        {
            LogEntry& e = s_log[i % kLogSize];
            fprintf(stderr, "#%u [tid%u] %-28s bucket=%p next=%p\n", i, e.tid, e.tag, e.bucket, e.nextVal);
        }
        fflush(stderr);
    }

    // Allocate memory
    void* Alloc(size_t size)
    {
        Bucket* triedBuckets = nullptr;
        Bucket* bucket;
        while (bucket = m_activeBuckets.Pop())
        {
            CheckOut(bucket, "POP_ACTIVE");
            DLog("POP_ACTIVE", bucket, bucket->next);

            if (bucket->GetLikelyUsed() == 0)
            {
                CheckIn(bucket, "PUSH_RESERVE(empty)");
                m_reserveBuckets.Push(bucket);
                DLog("PUSH_RESERVE(empty)", bucket, bucket->next);
                continue;
            }

            void* mem = bucket->Alloc(size);
            if (mem)
            {
                bucket->next = triedBuckets;
                triedBuckets = bucket;
                CheckInChain(bucket, "PUSHMANY_SUCCESS");
                m_activeBuckets.PushMany(bucket);
                DLog("PUSHMANY_SUCCESS", bucket, bucket->next);
                return mem;
            }

            bucket->next = triedBuckets;
            triedBuckets = bucket;
        }

        if (triedBuckets)
        {
            CheckInChain(triedBuckets, "PUSHMANY_TRIED");
            m_activeBuckets.PushMany(triedBuckets);
            DLog("PUSHMANY_TRIED", triedBuckets, triedBuckets->next);
        }

        bucket = m_reserveBuckets.Pop();
        if (!bucket)
            return nullptr;
        CheckOut(bucket, "POP_RESERVE");
        DLog("POP_RESERVE", bucket, bucket->next);
        void* mem = bucket->Alloc(size);
        CheckIn(bucket, "PUSH_ACTIVE(fallback)");
        m_activeBuckets.Push(bucket);
        DLog("PUSH_ACTIVE(fallback)", bucket, bucket->next);

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
        std::atomic<int> debugOwned{0};   // 0 = in a stack / available, 1 = checked out to a thread
        unsigned debugOwnerTid = 0;
    };

    static void CheckOut(Bucket* b, const char* tag)
    {
        int old = b->debugOwned.exchange(1, std::memory_order_seq_cst);
        if (old != 0)
        {
            fprintf(stderr, "\n!!!! DOUBLE CHECKOUT DETECTED at %s: bucket=%p was already owned by tid%u, now also by tid%u\n",
                tag, (void*)b, b->debugOwnerTid, Tid());
            DumpLog(tag, b);
        }
        b->debugOwnerTid = Tid();
    }

    static void CheckIn(Bucket* b, const char* tag)
    {
        int old = b->debugOwned.exchange(0, std::memory_order_seq_cst);
        if (old != 1)
        {
            fprintf(stderr, "\n!!!! CHECK-IN WITHOUT CHECKOUT at %s: bucket=%p tid%u\n", tag, (void*)b, Tid());
            DumpLog(tag, b);
        }
    }

    static void CheckInChain(Bucket* head, const char* tag)
    {
        for (Bucket* p = head; p; p = p->next)
            CheckIn(p, tag);
    }

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
