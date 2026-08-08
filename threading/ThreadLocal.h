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
        m_instances.Add(this);
    }
    ~ThreadLocalBase()
    {
        TlsFree(m_slot);
        EnterMutex emx(m_mx);
        m_instances.Remove(this);
    }

    virtual void Free() = 0;


    static void FreeAll()
    {
        EnterMutex emx(m_mx);
        for (int i=0; i<m_instances.GetCount(); i++)
        {
            m_instances[i]->Free();
        }

    }

protected:
    DWORD m_slot;

private:
    inline static Mutex m_mx;
    inline static List<ThreadLocalBase*> m_instances;
    friend class Thread;
};

template <typename T>
class ThreadLocal : public ThreadLocalBase
{
public:
    ThreadLocal()
    {
    }
    ~ThreadLocal()
    {
    }

    T* Get()
    {
    	T* p = (T*)TlsGetValue(m_slot);
        if (!p)
        {
            p = new T();
            TlsSetValue(m_slot, p);
        }
        return p;
    }

    virtual void Free() override
    {
    	T* p = (T*)TlsGetValue(m_slot);
        delete p;
    }
};



}

