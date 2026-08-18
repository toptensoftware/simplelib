#pragma once

#include <stdint.h>
#include <string.h>

#include "../Platform/Platform.h"

namespace SimpleLib
{

template <typename T>
class Atomic
{
    static_assert(sizeof(T) == 4 || sizeof(T) == sizeof(void*),
        "Only 32-bit and pointer-sized types supported");

protected:
    volatile T m_val;

public:
    Atomic() : m_val((T)0) {}
    explicit Atomic(T initial) : m_val(initial) {}

    Atomic(const Atomic&) = delete;
    Atomic& operator=(const Atomic&) = delete;

    T CompareExchange(T val, T compare)
    {
        if constexpr (sizeof(T) == sizeof(void*))
            return (T)Platform::atomicCompareExchange(
                (size_t volatile*)&m_val,
                (size_t)val,
                (size_t)compare);
        else
            return (T)Platform::atomicCompareExchange(
                (uint32_t volatile*)&m_val,
                (uint32_t)val,
                (uint32_t)compare);
    }

    // Same as CompareExchange, but returns whether it succeeded rather
    // than the previous value
    bool TrySet(T val, T compare)
    {
        return CompareExchange(val, compare) == compare;
    }

    T Get() const
    {
        if constexpr (sizeof(T) == sizeof(void*))
            return (T)Platform::atomicLoad((size_t volatile*)&m_val);
        else
            return (T)Platform::atomicLoad((uint32_t volatile*)&m_val);
    }
    
    T Set(T val)
    {
        if constexpr (sizeof(T) == sizeof(void*))
            return (T)Platform::atomicExchange(
                (size_t volatile*)&m_val,
                (size_t)val
            );
        else
            return (T)Platform::atomicExchange(
                (uint32_t volatile*)&m_val,
                (uint32_t)val
            );
    }

    T Inc()
    {
        if constexpr (sizeof(T) == sizeof(void*))
            return (T)Platform::atomicIncrement((size_t volatile *)&m_val);
        else
            return (T)Platform::atomicIncrement((uint32_t volatile*)&m_val);
    }

    T Dec()
    {
        if constexpr (sizeof(T) == sizeof(void*))
            return (T)Platform::atomicDecrement((size_t volatile *)&m_val);
        else
            return (T)Platform::atomicDecrement((uint32_t volatile*)&m_val);
    }

    T Add(T delta)
    {
        if constexpr (sizeof(T) == sizeof(void*))
            return (T)Platform::atomicAdd((size_t volatile*)&m_val, (size_t)delta);
        else
            return (T)Platform::atomicAdd((uint32_t volatile*)&m_val, (uint32_t)delta);
    }

    T FetchAdd(T delta)
    {
        if constexpr (sizeof(T) == sizeof(void*))
            return (T)Platform::atomicFetchAdd((size_t volatile*)&m_val, (size_t)delta);
        else
            return (T)Platform::atomicFetchAdd((uint32_t volatile*)&m_val, (uint32_t)delta);
    }


    // Block while current value == expected. Returns false on timeout.
    bool Wait(T expected, uint32_t timeout = kWaitForever)
    {
        T current = Get();
        while (current == expected)
        {
            if (!Platform::futexWait(const_cast<T*>(&m_val), &current, sizeof(T), timeout))
            {
                return false; // timed out (or, rarely, a real failure — see GetLastError)
            }
            current = Get(); // re-check — WaitOnAddress can wake spuriously
        }
        return true;
    }

    void WakeOne()
    {
        Platform::futexWakeOne(&m_val);
    }

    void WakeAll()
    {
        Platform::futexWakeAll(&m_val);
    }

};


// Specialization for float: the primary template's `(T)atomicLoad(...)`
// style casts would do a numeric conversion between float and uint32_t
// (eg: bit pattern 0x3f800000 becoming the float value 1065353216.0f)
// instead of reinterpreting the bits, which is never what's wanted for an
// atomic float. This bit-casts to/from uint32_t instead, so the stored
// bits are exactly the IEEE-754 representation of the float. Inc/Dec/Add
// aren't provided since incrementing the bit pattern isn't a meaningful
// operation on a float.
template <>
class Atomic<float>
{
protected:
    volatile uint32_t m_val;

    static uint32_t ToBits(float val) { uint32_t bits; memcpy(&bits, &val, sizeof(bits)); return bits; }
    static float ToFloat(uint32_t bits) { float val; memcpy(&val, &bits, sizeof(val)); return val; }

public:
    Atomic() : m_val(0) {}
    explicit Atomic(float initial) : m_val(ToBits(initial)) {}

    Atomic(const Atomic&) = delete;
    Atomic& operator=(const Atomic&) = delete;

    float CompareExchange(float val, float compare)
    {
        return ToFloat(Platform::atomicCompareExchange((uint32_t volatile*)&m_val, ToBits(val), ToBits(compare)));
    }

    // Same as CompareExchange, but returns whether it succeeded rather
    // than the previous value
    bool TrySet(float val, float compare)
    {
        return Platform::atomicCompareExchange((uint32_t volatile*)&m_val, ToBits(val), ToBits(compare)) == ToBits(compare);
    }

    float Get() const
    {
        return ToFloat(Platform::atomicLoad((uint32_t volatile*)&m_val));
    }

    float Set(float val)
    {
        return ToFloat(Platform::atomicExchange((uint32_t volatile*)&m_val, ToBits(val)));
    }

    // Block while current value == expected. Returns false on timeout.
    bool Wait(float expected, uint32_t timeout = kWaitForever)
    {
        uint32_t bits = ToBits(expected);
        uint32_t current = Platform::atomicLoad((uint32_t volatile*)&m_val);
        while (current == bits)
        {
            if (!Platform::futexWait(const_cast<uint32_t*>(&m_val), &current, sizeof(uint32_t), timeout))
            {
                return false; // timed out (or, rarely, a real failure — see GetLastError)
            }
            current = Platform::atomicLoad((uint32_t volatile*)&m_val); // re-check — WaitOnAddress can wake spuriously
        }
        return true;
    }

    void WakeOne()
    {
        Platform::futexWakeOne(&m_val);
    }

    void WakeAll()
    {
        Platform::futexWakeAll(&m_val);
    }
};


// A pointer paired with a monotonically incrementing generation tag, CAS'd
// together as a single 128-bit value. Plain pointer CAS can't tell "the
// same node I originally saw" from "that address, again, because it was
// popped and pushed back onto the stack while I was delayed between
// reading it and executing my CAS" (the ABA problem) - the tag changes on
// every successful TrySet, so a stale compare can never spuriously match
// even if the pointer cycles back to a previous value. Used for
// MpmcStack's head pointer, which reuses the same handful of nodes under
// high contention.
template <typename T>
class AtomicTaggedPtr
{
public:
    struct Value
    {
        T* ptr = nullptr;
        uint64_t tag = 0;

        bool operator==(const Value& other) const { return ptr == other.ptr && tag == other.tag; }
        bool operator!=(const Value& other) const { return !(*this == other); }
    };

    static_assert(sizeof(Value) == 16, "Value must pack to exactly 16 bytes for 128-bit CAS");

    AtomicTaggedPtr() = default;
    AtomicTaggedPtr(const AtomicTaggedPtr&) = delete;
    AtomicTaggedPtr& operator=(const AtomicTaggedPtr&) = delete;

    Value Get() const
    {
#if defined(_WIN32)
        long long cmp[2] = { 0, 0 };
        Platform::atomicCompareExchange128((volatile long long*)&m_val, 0, 0, cmp);
        Value v;
        memcpy(&v, cmp, sizeof(v));
        return v;
#else
        uint128_t raw = Platform::atomicLoad128((volatile uint128_t*)&m_val);
        Value v;
        memcpy(&v, &raw, sizeof(v));
        return v;
#endif
    }

    // Returns true if the value was `compare` and has been changed to `val`
    bool TrySet(Value val, Value compare)
    {
#if defined(_WIN32)
        long long ex[2], cmp[2];
        memcpy(ex, &val, sizeof(ex));
        memcpy(cmp, &compare, sizeof(cmp));
        return Platform::atomicCompareExchange128((volatile long long*)&m_val, ex[1], ex[0], cmp);
#else
        uint128_t ex, cmp, actual;
        memcpy(&ex, &val, sizeof(ex));
        memcpy(&cmp, &compare, sizeof(cmp));
        return Platform::atomicCompareExchange128((volatile uint128_t*)&m_val, ex, cmp, &actual);
#endif
    }

    // Non-atomic - only safe when no other thread can be concurrently
    // accessing this instance (eg: during Reset())
    void Set(Value val)
    {
        m_val = val;
    }

private:
    alignas(16) Value m_val;
};


} 