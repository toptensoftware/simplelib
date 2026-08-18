#pragma once

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <cassert>
#include <climits>

namespace SimpleLib::Platform
{

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

}