#pragma once

#include <Windows.h>
#include <intrin.h>
#include "../Core/Encoding.h"

#undef Yield        // #defined by Windows.h

#pragma comment(lib, "synchronization.lib")

using namespace SimpleLib;

namespace SimpleLib::Platform
{


inline int Win32ErrorToErrno(DWORD err)
{
    switch (err)
    {
        case ERROR_SUCCESS:             return 0;
        case ERROR_FILE_NOT_FOUND:      return ENOENT;
        case ERROR_PATH_NOT_FOUND:      return ENOENT;
        case ERROR_TOO_MANY_OPEN_FILES: return EMFILE;
        case ERROR_ACCESS_DENIED:       return EACCES;
        case ERROR_INVALID_HANDLE:      return EBADF;
        case ERROR_NOT_ENOUGH_MEMORY:   return ENOMEM;
        case ERROR_OUTOFMEMORY:         return ENOMEM;
        case ERROR_INVALID_DRIVE:       return ENODEV;
        case ERROR_CURRENT_DIRECTORY:   return EACCES;
        case ERROR_NOT_SAME_DEVICE:     return EXDEV;
        case ERROR_NO_MORE_FILES:       return ENOENT;
        case ERROR_WRITE_PROTECT:       return EROFS;
        case ERROR_BAD_UNIT:            return ENODEV;
        case ERROR_NOT_READY:           return ENXIO;
        case ERROR_SEEK:                return EINVAL;
        case ERROR_WRITE_FAULT:         return EIO;
        case ERROR_READ_FAULT:          return EIO;
        case ERROR_SHARING_VIOLATION:   return EACCES;
        case ERROR_LOCK_VIOLATION:      return EACCES;
        case ERROR_HANDLE_DISK_FULL:    return ENOSPC;
        case ERROR_NOT_SUPPORTED:       return ENOTSUP;
        case ERROR_FILE_EXISTS:         return EEXIST;
        case ERROR_ALREADY_EXISTS:      return EEXIST;
        case ERROR_CANNOT_MAKE:         return EACCES;
        case ERROR_INVALID_PARAMETER:   return EINVAL;
        case ERROR_DISK_FULL:           return ENOSPC;
        case ERROR_INVALID_NAME:        return ENOENT;
        case ERROR_DIR_NOT_EMPTY:       return ENOTEMPTY;
        case ERROR_BUSY:                return EBUSY;
        case ERROR_NOACCESS:            return EACCES;
        case ERROR_FILENAME_EXCED_RANGE:return ENAMETOOLONG;
        case ERROR_BAD_PATHNAME:        return ENOENT;
        case ERROR_DIRECTORY:           return ENOTDIR;
        case WAIT_TIMEOUT:              return ETIMEDOUT;
        case ERROR_NEGATIVE_SEEK:       return EINVAL;
        case ERROR_BROKEN_PIPE:         return EPIPE;
        case ERROR_OPERATION_ABORTED:   return ECANCELED;
        case ERROR_NOT_ENOUGH_QUOTA:    return ENOMEM;
        default:                        return EINVAL; // fallback
    }
}

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

// 128-bit (double-width) value, used for ABA-safe tagged pointers (see
// AtomicTaggedPtr below). MSVC has no __int128, so this is a 16-byte
// struct instead of a scalar (see the GCC/Clang typedef in lin.h); callers
// only ever memcpy through it, so the lack of arithmetic doesn't matter.
// {lo, hi} matches the word order _InterlockedCompareExchange128 expects
// for both its ComparandResult array and the Destination memory itself.
struct alignas(16) uint128_t
{
    unsigned long long lo;
    unsigned long long hi;
};

// 128-bit (double-width) CAS. Requires 16-byte alignment and a CPU
// supporting CMPXCHG16B (every x64 CPU in practice). On both success and
// failure, *actual is left holding the true current value - that's what
// makes this usable for a lock-free 128-bit load too (see atomicLoad128
// below: CAS with exchange == comparand is a no-op if it happens to
// match, and either way the actual current value comes back).
inline bool atomicCompareExchange128(volatile uint128_t* pval128, uint128_t val, uint128_t compare, uint128_t* actual)
{
    long long cmp[2] = { (long long)compare.lo, (long long)compare.hi };
    bool success = _InterlockedCompareExchange128((volatile long long*)pval128, (long long)val.hi, (long long)val.lo, cmp) != 0;
    actual->lo = (unsigned long long)cmp[0];
    actual->hi = (unsigned long long)cmp[1];
    return success;
}

inline uint128_t atomicLoad128(volatile uint128_t* pval128)
{
    uint128_t zero = { 0, 0 };
    uint128_t actual;
    atomicCompareExchange128(pval128, zero, zero, &actual);
    return actual;
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



// --------- IO ----------

typedef HANDLE TFile;

inline void Init(TFile& file)
{
    file = nullptr;
}

inline int Open(TFile& file, const char* filename, const char* mode)
{
    // Parse mode (r, r+, w, w+, a, a+), optional ignored 'b'
    char rwMode = 0;
    bool plus = false;
    while (mode[0])
    {
        if (mode[0] == 'r' || mode[0] == 'w' || mode[0] == 'a')
        {
            assert(rwMode == 0);
            rwMode = mode[0];
        }
        else if (mode[0] == 'b')
        {
        }
        else if (mode[0] == '+')
        {
            assert(!plus);
            plus = true;
        }
        else
        {
            assert(false && "invalid mode");
            return EINVAL;
        }
        mode++;
    }
    assert(rwMode != 0);
    if (rwMode == 0)
    {
        assert(false && "invalid mode");
        return EINVAL;
    }

    // Map mode
    DWORD dwAccess;
    DWORD dwShare;
    DWORD dwCreate;
    switch (rwMode)
    {
        case 'r':
            dwAccess = GENERIC_READ | (plus ? GENERIC_WRITE : 0);
            dwShare = FILE_SHARE_READ | FILE_SHARE_WRITE;
            dwCreate = OPEN_EXISTING;
            break;

        case 'w':
            dwAccess = GENERIC_WRITE | (plus ? GENERIC_READ : 0);
            dwShare = FILE_SHARE_READ | FILE_SHARE_WRITE;
            dwCreate = CREATE_ALWAYS;
            break;

        case 'a':
            dwAccess = GENERIC_WRITE | (plus ? GENERIC_READ : 0);
            dwShare = FILE_SHARE_READ | FILE_SHARE_WRITE;
            dwCreate = OPEN_ALWAYS;
            break;    
    }

    // Create file
    file = CreateFileW(Encode<wchar_t>(filename), dwAccess, dwShare, nullptr, dwCreate, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return Win32ErrorToErrno(GetLastError());

    // Append just seeks to end (unlike posix which always writes to end)
    if (rwMode == 'a')
    {
        LARGE_INTEGER li;
        li.QuadPart = 0;
        if (!SetFilePointerEx(file, li, nullptr, SEEK_END))
        {
            int err = Win32ErrorToErrno(GetLastError());
            CloseHandle(file);
            file = nullptr;
            return err;
        }
    }

    return 0;
}

inline int Close(TFile& file)
{
    if (!CloseHandle(file))
        return Win32ErrorToErrno(GetLastError());
    file = nullptr;
    return 0;
}

inline bool IsOpen(TFile& file)
{
    return file != nullptr;
}

inline int Read(TFile& file, void* pv, size_t cb, size_t* pcb)
{
    size_t cbLeft = cb;
    char* buf = (char*)pv;
    while (cbLeft)
    {
        // Limit this chunk
        size_t cbChunk = cbLeft;
        if (cbChunk > 0x8000000)
            cbChunk = 0x8000000;

        // Read it
        DWORD cbRead;
        if (!ReadFile(file, buf, (DWORD)cbChunk, &cbRead, nullptr))
        {
            return Win32ErrorToErrno(GetLastError());
        }

        // Update position/count
        cbLeft -= cbRead;
        buf += cbRead;

        // Full read not available?
        if (cbRead < cbChunk)
            break;
    }

    if (pcb)
    {
        // Return number of bytes read
        *pcb = cb - cbLeft;
    }
    else
    {
        // Check full number of byte read
        if (cbLeft)
            return EIO;
    }

    return 0;
}

inline int Write(TFile& file, const void* pv, size_t cb, size_t* pcb)
{
    size_t cbLeft = cb;
    const char* buf = (const char*)pv;
    while (cbLeft)
    {
        // Limit this chunk
        size_t cbChunk = cbLeft;
        if (cbChunk > 0x8000000)
            cbChunk = 0x8000000;

        // Read it
        DWORD cbWritten;
        if (!WriteFile(file, buf, (DWORD)cbChunk, &cbWritten, nullptr))
        {
            return Win32ErrorToErrno(GetLastError());
        }

        // Update position/count
        cbLeft -= cbWritten;
        buf += cbWritten;

        // Full write not available?
        if (cbWritten < cbChunk)
            break;
    }

    if (pcb)
    {
        // Return number of bytes read
        *pcb = cb - cbLeft;
    }
    else
    {
        // Check full number of byte read
        if (cbLeft)
            return EIO;
    }

    return 0;
}

inline int Seek(TFile& file, int64_t offset, int origin)
{
    LARGE_INTEGER li;
    li.QuadPart = offset;
    if (!SetFilePointerEx(file, li, nullptr, origin))
        return Win32ErrorToErrno(GetLastError());
    return 0;
}


inline int64_t Tell(TFile& file)
{
    LARGE_INTEGER distance;
    distance.QuadPart = 0;

    LARGE_INTEGER newPos;
    if (!SetFilePointerEx(file, distance, &newPos, FILE_CURRENT))
    {
        return -1;
    }

    return newPos.QuadPart;
}

inline int Truncate(TFile& file)
{
    if (!SetEndOfFile(file))
        return Win32ErrorToErrno(GetLastError());
    return 0;
}

inline int Flush(TFile& file)
{
    if (!FlushFileBuffers(file))
        return Win32ErrorToErrno(GetLastError());
    return 0;
}




}


