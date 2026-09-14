#pragma once

#include <assert.h>
#include "Atomic.h"
#include "../Core/FourCC.h"

#include <malloc.h>

namespace SimpleLib
{

// Implementation of a non-blocking thread-safe "highwater" heap
// Heap fills up until everything has been freed and then
// resets to completely empty
// Intended to be used for short term, non-blocking memory allocations
class HighWaterHeap
{
public:
    // Constructor
    HighWaterHeap(uint32_t capacity)
    {
        Reset(capacity);
    }

    // Destructor
    virtual ~HighWaterHeap()
    {
        free(m_pmem);
    }
    
    // Resize and reset the heap to a new capacity
    void Reset(uint32_t capacity)
    {
        if (capacity != m_capacity)
        {
            free(m_pmem);
            m_pmem = (char*)malloc(capacity);
            m_capacity = capacity;
        }

        m_state.Set(0);
    }

    // Immediately free everything from the heap
    void FreeAll()
    {
        m_state.Set(0);
    }

    // Allocate memory from the heap
    // Returns nullptr on failure.
    void* Alloc(uint32_t bytes)
    {
        // Remember the caller's requested size (for GetSize()) before
        // padding it out with header and rounding overhead
        uint32_t requestedBytes = bytes;

        // Work out actual required bytes for the allocation
        bytes += sizeof(HEADER);
        bytes = (bytes + 15) & ~(0xFL);

        while (true)
        {
            // Get state
            STATE state;
            state.packed = m_state.Get();

            // Full?
            if (bytes + state.unpacked.high > m_capacity)
            {
                OnFull();
                return nullptr;
            }

            // Update state
            STATE newState;
            newState.unpacked.count = state.unpacked.count+1;
            newState.unpacked.high = (uint32_t)(state.unpacked.high + bytes);

            // Claim it
            if (!m_state.TrySet(newState.packed, state.packed) )
                continue;

            // Setup header
            HEADER* p = (HEADER*)(m_pmem + state.unpacked.high);
            p->heap = this;
            p->size = requestedBytes;
            p->sig = kSigAlloc;

            // Notify if this allocation just crossed the trigger level
            uint32_t trigger = m_trigger.Get();
            if (state.unpacked.high <= trigger && newState.unpacked.high > trigger)
                OnTrigger();

            // Return user block
            return (void*)(p+1);
        }
    }

    // Frees memory previously allocated from the heap
    void Free(void* mem)
    {
        // Get header
        HEADER* p = ((HEADER*)mem) - 1;
        assert(p->sig == kSigAlloc);
        assert(p->heap->m_sig == kSigHeap);

        // Check for double free/invalid block
        assert(p->heap == this);
        p->heap = nullptr;

        while (true)
        {
            // Get state
            STATE state;
            state.packed = m_state.Get();

            // Work out new state
            STATE newState;
            if (state.unpacked.count == 1)
            {
                newState.unpacked.count = 0;
                newState.unpacked.high = 0;
            }
            else
            {
                newState.unpacked.count = state.unpacked.count - 1;
                newState.unpacked.high = state.unpacked.high;
            }

            // Claim
            if (!m_state.TrySet(newState.packed, state.packed))
                continue;

            // Notify
            if (newState.unpacked.count == 0)
                OnEmpty();

            return;
        }
    }

    // Get size of an allocation
    static uint32_t GetSize(void* mem)
    {
        // Get header
        HEADER* p = ((HEADER*)mem) - 1;
        assert(p->sig == kSigAlloc);
        assert(p->heap->m_sig == kSigHeap);
        return p->size;
    }

    // Check how much of the heap has been used
    uint32_t GetLikelyUsed()
    {
        STATE state;
        state.packed = m_state.Get();
        return state.unpacked.high;
    }

    // Check how much of the heap has been used
    uint32_t GetLikelyCount()
    {
        STATE state;
        state.packed = m_state.Get();
        return state.unpacked.count;
    }

    // Get the heap's capacity
    uint32_t GetCapacity()
    {
        return m_capacity;
    }

    // Notification when the heap has been emptied
    // (Typically used to move a previously full out of rotation
    // heap back into rotation)
    virtual void OnEmpty()
    {
    }

    // Notification when an allocation couldn't be satisfied because
    // there wasn't enough room left in the heap. May be called more than
    // once while the heap remains full.
    virtual void OnFull()
    {
    }

    // Set the trigger level - once the high-water mark passes this many
    // bytes used, OnTrigger() is called (once per fill cycle, on the
    // allocation that causes the crossing)
    void SetTrigger(uint32_t mark)
    {
        m_trigger.Set(mark);
    }

    // Notification when the high-water mark passes the trigger level
    // set with SetTrigger() (default trigger level is 0, ie: triggers on
    // the first allocation of each fill cycle)
    virtual void OnTrigger()
    {
    }

    // Get the highwater heap a pointer was allocated from
    static HighWaterHeap* FromAllocation(void* mem)
    {
        HEADER* p = ((HEADER*)mem) - 1;
        assert(p->sig == kSigAlloc);
        assert(p->heap->m_sig == kSigHeap);
        return p->heap;
    }

    // Free a previous allocation from the correct heap
    static void FreeFromCorrectHeap(void* mem)
    {
        if (mem == nullptr)
            return;
        FromAllocation(mem)->Free(mem);
    }


private:
#pragma pack(1)
    union STATE
    {
        struct
        {
            uint32_t count;
            uint32_t high;
        } unpacked;
        uint64_t packed;
    };
#pragma pack()
    static_assert(sizeof(STATE) == sizeof(uint64_t), "state size doesn't match uint64_t");

    uint32_t m_sig = kSigHeap;
    Atomic<uint64_t> m_state;   // STATE::packed
    Atomic<uint32_t> m_trigger; // defaults to 0
    uint32_t m_capacity = 0;
    char* m_pmem = nullptr;
    struct HEADER
    {
        HighWaterHeap* heap;
        uint32_t size;
        uint32_t sig;
    };

    // No move or copy
    HighWaterHeap(const HighWaterHeap&) = delete;
	HighWaterHeap& operator=(const HighWaterHeap&) = delete;		
	HighWaterHeap(HighWaterHeap&& other) = delete;
	HighWaterHeap& operator=(HighWaterHeap&& other) = delete;

    static const uint32_t kSigAlloc = cc4("hwal");
    static const uint32_t kSigHeap = cc4("hwhp");
};

}

