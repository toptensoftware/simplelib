#pragma once

#include "Mutex.h"

namespace SimpleLib
{

class ThreadLocalBase
{
public:
    ThreadLocalBase()
    {
        m_slot = TlsAlloc();
        EnterMutex emx(m_mx);
        m_slots.Add(this);
    }
    ~ThreadLocalBase()
    {
        TlsFree(m_slot);
        EnterMutex emx(m_mx);
        m_slots.Remove(this);
    }

    virtual void Free() = 0;

    // Free all TLS data types for the current thread
    // (Called automatically when SimpleLib Thread ends)
    static void FreeAll()
    {
        EnterMutex emx(m_mx);
        for (int i=0; i<m_slots.GetCount(); i++)
        {
            m_slots[i]->Free();
        }
    }

protected:
    uint32_t m_slot;

private:
    inline static Mutex m_mx;
    inline static List<ThreadLocalBase*> m_slots;
    friend class Thread;
};

template <typename T, bool autoCreate = true>
class ThreadLocal : public ThreadLocalBase
{
public:
    ThreadLocal()
    {
    }
    ~ThreadLocal()
    {
    }

    // Get a per-thread object instance for this thread
    // (Creates instance if not already set)
    T* Get()
    {
        T* p = (T*)TlsGetValue(m_slot);
        if constexpr (autoCreate)
        {
            if (!p)
            {
                p = Set(new T());
            }
        }
        return p;
    }

    // Sets the per-thread instance data for this thread
    // (Note: old instance won't be automatically deleted)
    T* Set(T* val)
    {
        TlsSetValue(m_slot, (void*)val);
        return val;
    }

    // Delete the thread data associated with the current thread
    virtual void Free() override
    {
        if constexpr (autoCreate)
        {
            T* p = (T*)TlsGetValue(m_slot);
            delete p;
        }
    }
};  



}

