#pragma once

#include <stdint.h>
#include "common.h"
    
namespace SimpleLib
{

#if defined(_WIN32)

#include <Windows.h>

#pragma comment(lib, "synchronization.lib")

inline size_t atomicCompareExchange(volatile size_t* pval, size_t val, size_t compare)
{
    return (size_t)InterlockedCompareExchangePointer((void* volatile*)pval, (void*)val, (void*)compare);
}

inline uint32_t atomicCompareExchange(volatile uint32_t* pval, uint32_t val, uint32_t compare)
{
    return (uint32_t)InterlockedCompareExchange((volatile LONG*)pval, (LONG)val, (LONG)compare);
}

inline size_t atomicExchange(volatile size_t* pval, size_t val)
{
    return (size_t)InterlockedExchangePointer((void* volatile*)pval, (void*)val);
}

inline uint32_t atomicExchange(volatile uint32_t* pval, uint32_t val)
{
    return (uint32_t)InterlockedExchange((volatile LONG*)pval, (LONG)val);
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

inline size_t atomicExchange(volatile size_t* pval, size_t val)
{
    return __atomic_exchange_n(pval, val, __ATOMIC_SEQ_CST);
}

inline uint32_t atomicExchange(volatile uint32_t* pval, uint32_t val)
{
    return __atomic_exchange_n(pval, val, __ATOMIC_SEQ_CST);
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

    T TrySet(T val, T compare)
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

    T Get() const
    {
        return m_val;
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


} 