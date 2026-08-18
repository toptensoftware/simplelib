#pragma once

#include <Windows.h>
#include <intrin.h>
#include "../Core/Encoding.h"

#undef Yield        // #defined by Windows.h

#pragma comment(lib, "synchronization.lib")

using namespace SimpleLib;

namespace SimpleLib::Platform
{

// --------- Threading ----------

inline void Yield()
{
    ::YieldProcessor();
}

inline void Sleep(uint32_t ms)
{
    ::Sleep(ms);
}

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


typedef CRITICAL_SECTION TMutex;

inline void mutexCreate(TMutex& mutex)
{
    InitializeCriticalSection(&mutex);
    SetCriticalSectionSpinCount(&mutex, 4000);
}

inline void mutexDestroy(TMutex& mutex)
{
    DeleteCriticalSection(&mutex);
}

inline void mutexEnter(TMutex& mutex)
{
    EnterCriticalSection(&mutex);
}

inline bool mutexTryEnter(TMutex& mutex)
{
    return TryEnterCriticalSection(&mutex);
}

inline void mutexLeave(TMutex& mutex)
{
    LeaveCriticalSection(&mutex);
}

inline bool mutexIsHeld(TMutex& mutex)
{
    return mutex.OwningThread == (HANDLE)(size_t)GetCurrentThreadId();
}


typedef HANDLE TSema;

inline void semaCreate(TSema& sema, int initialValue)
{
    sema = CreateSemaphore(nullptr, initialValue, 0x7FFFFFFF, nullptr);
}

inline void semaDestroy(TSema& sema)
{
    CloseHandle(sema);
    sema = nullptr;
}

inline bool semaWait(TSema& sema, uint32_t timeout)
{
    return WaitForSingleObject(sema, timeout) == WAIT_OBJECT_0;
}

inline void semaRelease(TSema& sema, int count)
{
    ReleaseSemaphore(sema, count, nullptr);
}


typedef SRWLOCK TSlimlock;

inline void slimlockCreate(TSlimlock& slimlock)
{
    InitializeSRWLock(&slimlock);
}

inline void slimlockDestroy(TSlimlock& slimlock)
{
}

inline void slimlockEnterExclusive(TSlimlock& slimlock)
{
    AcquireSRWLockExclusive(&slimlock);
}

inline bool slimlockTryEnterExclusive(TSlimlock& slimlock)
{
    return TryAcquireSRWLockExclusive(&slimlock);
}

inline void slimlockLeaveExclusive(TSlimlock& slimlock)
{
    ReleaseSRWLockExclusive(&slimlock);
}

inline void slimlockEnterShared(TSlimlock& slimlock)
{
    AcquireSRWLockShared(&slimlock);
}

inline bool slimlockTryEnterShared(TSlimlock& slimlock)
{
    return TryAcquireSRWLockShared(&slimlock);
}

inline void slimlockLeaveShared(TSlimlock& slimlock)
{
    ReleaseSRWLockShared(&slimlock);
}


typedef HANDLE TThread;

inline void threadInit(TThread& thread)
{
    thread = nullptr;
}

inline void threadDestroy(TThread& thread)
{
    if (thread)
        CloseHandle(thread);
}

inline bool threadIsCreated(TThread& thread)
{
    return thread != nullptr;
}

struct ThreadProcParam
{
    ThreadProc proc;
    void* param;
};

inline DWORD WINAPI ThreadProcStub(void* param)
{
    ThreadProcParam* tpp = (ThreadProcParam*)param;
    tpp->proc(tpp->param);
    delete tpp;
    return 0;
}

inline size_t threadStart(TThread& thread, ThreadProc proc, void* param)
{
    ThreadProcParam* tpp = new ThreadProcParam();
    tpp->proc = proc;
    tpp->param = param;

    DWORD dwId;
    thread = CreateThread(nullptr, 0, &ThreadProcStub, tpp, CREATE_SUSPENDED, &dwId);
    return dwId;
}

inline void threadResume(TThread& thread)
{
    ResumeThread(thread);
}

inline bool threadJoin(TThread& thread, uint32_t timeout)
{
    // Not created?
    if (!thread)
        return true;

    // Wait
    if (WaitForSingleObject(thread, timeout) != WAIT_OBJECT_0)
        return false;

    // Close
    CloseHandle(thread);
    thread = nullptr;
    return true;
}

inline void threadSetDescription(TThread thread, const char* value)
{
    assert(thread != nullptr);
    SetThreadDescription(thread, Encode<wchar_t>(value).sz());
}

inline void threadSetPriority(TThread thread, ThreadPriority priority)
{
    assert(thread != nullptr);
    switch (priority)
    {
        case ThreadPriority::RealTime:
            // High priority
            SetThreadPriority(thread, THREAD_PRIORITY_TIME_CRITICAL);
            break;

        case ThreadPriority::Normal:
            SetThreadPriority(thread, THREAD_PRIORITY_NORMAL);
            break;

        case ThreadPriority::AboveNormal:
            SetThreadPriority(thread, THREAD_PRIORITY_ABOVE_NORMAL);
            break;

        case ThreadPriority::BelowNormal:
            SetThreadPriority(thread, THREAD_PRIORITY_BELOW_NORMAL);
            break;
    }
}

inline size_t threadCurrentId()
{
    return (size_t)GetCurrentThreadId();
}

inline TThread threadCurrentHandle()
{
    return GetCurrentThread();
}


typedef uint32_t TTls;

inline void tlsAlloc(TTls& tls)
{
    tls = TlsAlloc();
}

inline void tlsFree(TTls& tls)
{
    TlsFree(tls);
    tls = 0;
}

inline void* tlsGet(TTls& tls)
{   
    return TlsGetValue(tls);
}

inline void tlsSet(TTls& tls, void* val)
{
    TlsSetValue(tls, val);
}

// --------- File System ----------

inline bool FileExists(const char* filename)
{
    return _waccess(Encode<wchar_t>(filename), 0) == 0;
}

inline String GetTempDirectory()
{
    wchar_t sz[MAX_PATH];
    GetTempPathW(MAX_PATH, sz);
    return Encode<char>(sz);
}

inline int Rename(const char* oldName, const char* newName)
{
    return _wrename(Encode<wchar_t>(oldName), Encode<wchar_t>(newName));
}

inline int Unlink(const char* filename)
{
    return _wunlink(Encode<wchar_t>(filename));
}


inline int Stat(const char* pszFileName, Stat* stat)
{
    struct _stat64 s;
    int retv = _wstati64(Encode<wchar_t>(pszFileName), &s);
    stat->size = s.st_size;
    stat->mtime = s.st_mtime;
    return retv;
}

inline int MakeDirectory(const char* path)
{
    return _wmkdir(Encode<wchar_t>(path));
}


}


