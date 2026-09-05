#pragma once

#include "atomic.h"

namespace SimpleLib
{

// A pure busy-wait lock: never parks the thread with the OS. Only worth
// using for very short critical sections under light contention - for
// anything longer, use SlimLock/Mutex instead so waiters block rather than
// burn CPU.
//
// alignas(64) keeps it off a cache line shared with whatever else sits next
// to it - important when a caller (eg: a real-time audio thread) is going
// to TryEnter() every cycle regardless of contention.
class alignas(64) SpinLock
{
public:
    SpinLock() : m_locked(false) {}

    SpinLock(const SpinLock&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;

    void Enter()
    {
        for (;;)
        {
            if (TryEnter())
                return;

            // Spin on a plain load (not CAS) while contended, so we're not
            // hammering the cache line with exclusive-ownership requests
            // while some other core holds the lock.
            while (m_locked.Get())
                Platform::Yield();
        }
    }

    bool TryEnter()
    {
        return m_locked.TrySet(true, false);
    }

    void Leave()
    {
        m_locked.Set(false);
    }

private:
    Atomic<bool> m_locked;
};

class EnterSpinLock
{
public:
    EnterSpinLock()
    {
        m_pSpinLock = nullptr;
    }

    EnterSpinLock(SpinLock& spinlock)
    {
        m_pSpinLock = nullptr;
        Enter(spinlock);
    }

    ~EnterSpinLock()
    {
        if (m_pSpinLock != nullptr)
            Leave();
    }

    void Enter(SpinLock& spinlock)
    {
        assert(m_pSpinLock == nullptr);
        m_pSpinLock = &spinlock;
        m_pSpinLock->Enter();
    }

    bool TryEnter(SpinLock& spinlock)
    {
        assert(m_pSpinLock == nullptr);
        if (spinlock.TryEnter())
        {
            m_pSpinLock = &spinlock;
            return true;
        }
        return false;
    }

    void Leave()
    {
        assert(m_pSpinLock != nullptr);
        m_pSpinLock->Leave();
        m_pSpinLock = nullptr;
    }

    SpinLock* m_pSpinLock;
};

}
