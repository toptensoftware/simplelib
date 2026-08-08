#pragma once

#include "common.h"
#include "Atomic.h"


namespace SimpleLib
{

class AtomicSemaphore
{
    Atomic<uint32_t> m_count;
    Atomic<uint32_t> m_stop;

public:
    AtomicSemaphore(uint32_t initialValue) : 
        m_count(initialValue),
        m_stop(0)
    {
    }

    // Producer: add N signals, wake up to N waiters
    void Release(uint32_t n = 1)
    {
        m_count.Add(n);
        while (n-- > 0)
            m_count.WakeOne();
    }

    // Consumer: block until a signal is available, then consume one
    bool Wait(uint32_t timeout = kWaitForever)
    {
        return SpinWait(0, timeout);
    }

    // Consumer: as Wait, but busy-spins up to spinCount times before
    // falling through to the OS wait. Useful when releases are expected
    // to arrive quickly, where the syscall/context-switch cost of the OS
    // wait would otherwise dominate.
    bool SpinWait(uint32_t spinCount, uint32_t timeout = kWaitForever)
    {
        while (true)
        {
            if (m_stop.Get())
                return false;

            uint32_t current = m_count.Get();
            while (current > 0)
            {
                uint32_t prev = m_count.TrySet(current - 1, current);
                if (prev == current)
                    return true; // successfully claimed one unit
                current = m_count.Get(); // CAS failed, retry with fresh value
            }

            // current == 0 — busy-spin for a bit before sleeping
            for (uint32_t spins = spinCount; spins > 0 && current == 0; spins--)
            {
                YieldProcessor();
                current = m_count.Get();
            }
            if (current > 0)
                continue; // spun into a signal — go back and try to claim it

            // still zero after spinning — go to sleep until count changes
            if (!m_count.Wait(current, timeout))
                return false; // timeout

            // woke up — loop back and re-check/try to claim
        }
    }

    void Stop()
    {
        m_stop.Set(1);
        m_count.Add(1);   // force a real value change so any parked CAtomic::Wait() unblocks
        m_count.WakeAll();
    }

    // Caller must ensure all waiter threads have actually returned from
    // Wait/SpinWait (e.g. joined) before calling Reset — calling it while a
    // Stop() wakeup is still in flight can race a waiter back to sleep
    // before it observes the stop.
    void Reset()
    {
        m_stop.Set(0);
        m_count.Set(0);
    }
};

}