#pragma once

#include <stdint.h>
#include <string.h>
#include "common.h"
    
namespace SimpleLib
{

#if defined(_WIN32)

#include <Windows.h>
#include <intrin.h>

#pragma comment(lib, "synchronization.lib")

inline size_t atomicCompareExchange(volatile size_t* pval, size_t val, size_t compare)
{
    return (size_t)InterlockedCompareExchangePointer((void* volatile*)pval, (void*)val, (void*)compare);
}

inline uint32_t atomicCompareExchange(volatile uint32_t* pval, uint32_t val, uint32_t compare)
{
    return (uint32_t)InterlockedCompareExchange((volatile LONG*)pval, (LONG)val, (LONG)compare);
}

// 128-bit (double-width) CAS, used for ABA-safe tagged pointers (see
// AtomicTaggedPtr below). Requires 16-byte alignment and a CPU supporting
// CMPXCHG16B (every x64 CPU in practice). `pval128` is treated as two
// consecutive 64-bit words {low, high}. On both success and failure,
// *pval128 is left holding the actual current value - that's what makes
// this usable for a lock-free 128-bit load too (CAS with exchange ==
// comparand: a no-op if it happens to match, and either way the compare
// buffer is updated to the true current value).
inline bool atomicCompareExchange128(volatile long long* pval128, long long exchangeHigh, long long exchangeLow, long long* comparand /* [2]: {low, high}, updated in place */)
{
    return _InterlockedCompareExchange128(pval128, exchangeHigh, exchangeLow, comparand) != 0;
}

inline size_t atomicExchange(volatile size_t* pval, size_t val)
{
    return (size_t)InterlockedExchangePointer((void* volatile*)pval, (void*)val);
}

inline uint32_t atomicExchange(volatile uint32_t* pval, uint32_t val)
{
    return (uint32_t)InterlockedExchange((volatile LONG*)pval, (LONG)val);
}

// A plain read of a volatile, naturally-aligned size_t/uint32_t is already
// atomic in hardware on x86/x64, and MSVC's default (non-ISO) volatile
// semantics give it acquire ordering - matching the release semantics of
// the Interlocked* stores above - so this is a real fenced load here, not
// just a cache of a stale value. This relies on MSVC-specific behavior
// (not in effect under /volatile:iso); see the Linux branch below for the
// portable equivalent using __atomic_load_n.
inline size_t atomicLoad(volatile size_t* pval)
{
    return *pval;
}

inline uint32_t atomicLoad(volatile uint32_t* pval)
{
    return *pval;
}

inline size_t atomicIncrement(volatile size_t* pval)
{
#if defined(_WIN64)
    return (size_t)InterlockedIncrement64((volatile LONGLONG*)pval);
#else
    return (size_t)InterlockedIncrement((volatile LONG*)pval);
#endif
}

inline uint32_t atomicIncrement(volatile uint32_t* pval)
{
    return (uint32_t)InterlockedIncrement((volatile LONG*)(pval));
}

inline size_t atomicDecrement(volatile size_t* pval)
{
#if defined(_WIN64)
    return (size_t)InterlockedDecrement64((volatile LONGLONG*)pval);
#else
    return (size_t)InterlockedDecrement((volatile LONG*)pval);
#endif
}

inline uint32_t atomicDecrement(volatile uint32_t* pval)
{
    return (uint32_t)InterlockedDecrement((volatile LONG*)pval);
}

inline size_t atomicFetchAdd(volatile size_t* pval, size_t delta)
{
#if defined(_WIN64)
    return (size_t)InterlockedExchangeAdd64((volatile LONGLONG*)pval, (LONGLONG)delta);
#else
    return (size_t)InterlockedExchangeAdd((volatile LONG*)pval, (LONG)delta);
#endif
}

inline uint32_t atomicFetchAdd(volatile uint32_t* pval, uint32_t delta)
{
    return (uint32_t)InterlockedExchangeAdd((volatile LONG*)pval, (LONG)delta);
}

inline size_t atomicAdd(volatile size_t* pval, size_t delta)
{
    return atomicFetchAdd(pval, delta) + delta;
}

inline uint32_t atomicAdd(volatile uint32_t* pval, uint32_t delta)
{
    return atomicFetchAdd(pval, delta) + delta;
}


inline bool futexWait(volatile void* pv, const void* pcompare, size_t size, uint32_t timeout = kWaitForever)
{
    return WaitOnAddress(
        pv, 
        (void*)pcompare, 
        size, 
        timeout
    );
}

inline void futexWakeOne(volatile void* pv)
{
    WakeByAddressSingle((void*)pv);
}

inline void futexWakeAll(volatile void* pv)
{
    WakeByAddressAll((void*)pv);
}


#else // Linux / GCC / Clang

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <cassert>
#include <climits>

inline long sys_futex(void* addr1, int op, uint32_t val1, const struct timespec* timeout, void* addr2, uint32_t val3)
{
    return syscall(SYS_futex, addr1, op, val1, timeout, addr2, val3);
}

inline size_t atomicCompareExchange(volatile size_t* pval, size_t val, size_t compare)
{
    __atomic_compare_exchange_n(pval, &compare, val, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return compare; // holds the pre-op value either way
}

inline uint32_t atomicCompareExchange(volatile uint32_t* pval, uint32_t val, uint32_t compare)
{
    __atomic_compare_exchange_n(pval, &compare, val, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return compare;
}

// 128-bit (double-width) CAS, used for ABA-safe tagged pointers (see
// AtomicTaggedPtr below). Requires 16-byte alignment and -mcx16 (CMPXCHG16B)
// on x86-64; __int128 is a GCC/Clang extension available on 64-bit targets.
typedef unsigned __int128 uint128_t;

inline bool atomicCompareExchange128(volatile uint128_t* pval128, uint128_t val, uint128_t compare, uint128_t* actual)
{
    bool success = __atomic_compare_exchange_n(pval128, &compare, val, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    *actual = compare; // updated to the true current value on failure; on success it's unchanged (still the value we matched)
    return success;
}

inline uint128_t atomicLoad128(volatile uint128_t* pval128)
{
    return __atomic_load_n(pval128, __ATOMIC_SEQ_CST);
}

inline size_t atomicExchange(volatile size_t* pval, size_t val)
{
    return __atomic_exchange_n(pval, val, __ATOMIC_SEQ_CST);
}

inline uint32_t atomicExchange(volatile uint32_t* pval, uint32_t val)
{
    return __atomic_exchange_n(pval, val, __ATOMIC_SEQ_CST);
}

inline size_t atomicLoad(volatile size_t* pval)
{
    return __atomic_load_n(pval, __ATOMIC_SEQ_CST);
}

inline uint32_t atomicLoad(volatile uint32_t* pval)
{
    return __atomic_load_n(pval, __ATOMIC_SEQ_CST);
}

inline size_t atomicIncrement(volatile size_t* pval)
{
    return __atomic_add_fetch(pval, 1, __ATOMIC_SEQ_CST);
}

inline uint32_t atomicIncrement(volatile uint32_t* pval)
{
    return __atomic_add_fetch(pval, 1, __ATOMIC_SEQ_CST);
}

inline size_t atomicDecrement(volatile size_t* pval)
{
    return __atomic_sub_fetch(pval, 1, __ATOMIC_SEQ_CST);
}

inline uint32_t atomicDecrement(volatile uint32_t* pval)
{
    return __atomic_sub_fetch(pval, 1, __ATOMIC_SEQ_CST);
}

inline size_t atomicFetchAdd(volatile size_t* pval, size_t delta)
{
    return __atomic_fetch_add(pval, delta, __ATOMIC_SEQ_CST);
}

inline uint32_t atomicFetchAdd(volatile uint32_t* pval, uint32_t delta)
{
    return __atomic_fetch_add(pval, delta, __ATOMIC_SEQ_CST);
}

inline size_t atomicAdd(volatile size_t* pval, size_t delta)
{
    return __atomic_add_fetch(pval, delta, __ATOMIC_SEQ_CST);
}

inline uint32_t atomicAdd(volatile uint32_t* pval, uint32_t delta)
{
    return __atomic_add_fetch(pval, delta, __ATOMIC_SEQ_CST);
}

inline bool futexWait(volatile void* pv, const void* pcompare, size_t size, uint32_t timeout = kWaitForever)
{
    assert(size == 4 && "futexWait: only 32-bit values supported on Linux (no native 64-bit futex)");

    struct timespec ts;
    struct timespec* pts = nullptr;
    if (timeout != kWaitForever)
    {
        ts.tv_sec  = timeout / 1000;
        ts.tv_nsec = (long)(timeout % 1000) * 1000000L;
        pts = &ts;
    }

    uint32_t compareVal;
    memcpy(&compareVal, pcompare, sizeof(compareVal));

    long ret = sys_futex((void*)pv, FUTEX_WAIT, compareVal, pts, nullptr, 0);
    if (ret == 0)
        return true;
    if (errno == EAGAIN || errno == EINTR)
        return true;  // value already differed, or spurious wake — caller re-checks
    return false;      // ETIMEDOUT or genuine error
}

inline void futexWakeOne(volatile void* pv)
{
    sys_futex((void*)pv, FUTEX_WAKE, 1, nullptr, nullptr, 0);
}

inline void futexWakeAll(volatile void* pv)
{
    sys_futex((void*)pv, FUTEX_WAKE, (uint32_t)INT_MAX, nullptr, nullptr, 0);
}

#endif

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
            return (T)atomicCompareExchange(
                (size_t volatile*)&m_val,
                (size_t)val,
                (size_t)compare);
        else
            return (T)atomicCompareExchange(
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
            return (T)atomicLoad((size_t volatile*)&m_val);
        else
            return (T)atomicLoad((uint32_t volatile*)&m_val);
    }
    
    T Set(T val)
    {
        if constexpr (sizeof(T) == sizeof(void*))
            return (T)atomicExchange(
                (size_t volatile*)&m_val,
                (size_t)val
            );
        else
            return (T)atomicExchange(
                (uint32_t volatile*)&m_val,
                (uint32_t)val
            );
    }

    T Inc()
    {
        if constexpr (sizeof(T) == sizeof(void*))
            return (T)atomicIncrement((size_t volatile *)&m_val);
        else
            return (T)atomicIncrement((uint32_t volatile*)&m_val);
    }

    T Dec()
    {
        if constexpr (sizeof(T) == sizeof(void*))
            return (T)atomicDecrement((size_t volatile *)&m_val);
        else
            return (T)atomicDecrement((uint32_t volatile*)&m_val);
    }

    T Add(T delta)
    {
        if constexpr (sizeof(T) == sizeof(void*))
            return (T)atomicAdd((size_t volatile*)&m_val, (size_t)delta);
        else
            return (T)atomicAdd((uint32_t volatile*)&m_val, (uint32_t)delta);
    }

    T FetchAdd(T delta)
    {
        if constexpr (sizeof(T) == sizeof(void*))
            return (T)atomicFetchAdd((size_t volatile*)&m_val, (size_t)delta);
        else
            return (T)atomicFetchAdd((uint32_t volatile*)&m_val, (uint32_t)delta);
    }


    // Block while current value == expected. Returns false on timeout.
    bool Wait(T expected, uint32_t timeout = kWaitForever)
    {
        T current = Get();
        while (current == expected)
        {
            if (!futexWait(const_cast<T*>(&m_val), &current, sizeof(T), timeout))
            {
                return false; // timed out (or, rarely, a real failure — see GetLastError)
            }
            current = Get(); // re-check — WaitOnAddress can wake spuriously
        }
        return true;
    }

    void WakeOne()
    {
        futexWakeOne(&m_val);
    }

    void WakeAll()
    {
        futexWakeAll(&m_val);
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
        atomicCompareExchange128((volatile long long*)&m_val, 0, 0, cmp);
        Value v;
        memcpy(&v, cmp, sizeof(v));
        return v;
#else
        uint128_t raw = atomicLoad128((volatile uint128_t*)&m_val);
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
        return atomicCompareExchange128((volatile long long*)&m_val, ex[1], ex[0], cmp);
#else
        uint128_t ex, cmp, actual;
        memcpy(&ex, &val, sizeof(ex));
        memcpy(&cmp, &compare, sizeof(cmp));
        return atomicCompareExchange128((volatile uint128_t*)&m_val, ex, cmp, &actual);
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