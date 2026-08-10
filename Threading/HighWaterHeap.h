#pragma once

#include <assert.h>
#include "Atomic.h"

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
    void* Alloc(size_t bytes)
    {
        // Work out actual required bytes for the allocation
        bytes += sizeof(HEADER);
        bytes = (bytes + 15) & ~(0xFLL);

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

    // Check how much of the heap has been used
    size_t GetLikelyUsed()
    {
        STATE state;
        state.packed = m_state.Get();
        return state.unpacked.high;
    }

    // Check how much of the heap has been used
    size_t GetLikelyCount()
    {
        STATE state;
        state.packed = m_state.Get();
        return state.unpacked.count;
    }

    // Get the heap's capacity
    size_t GetCapacity()
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
        return (((HEADER*)mem)-1)->heap;
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

    Atomic<uint64_t> m_state;   // STATE::packed
    Atomic<uint32_t> m_trigger; // defaults to 0
    uint32_t m_capacity = 0;
    char* m_pmem = nullptr;
    struct HEADER
    {
        HighWaterHeap* heap;
    };

    // No move or copy
    HighWaterHeap(const HighWaterHeap&) = delete;
	HighWaterHeap& operator=(const HighWaterHeap&) = delete;		
	HighWaterHeap(HighWaterHeap&& other) = delete;
	HighWaterHeap& operator=(HighWaterHeap&& other) = delete;
};

}

