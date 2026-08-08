#pragma once

#include "mutex.h"

namespace SimpleLib
{

template <typename T>
class Tls
{
public:
    Tls()
    {
        m_slot = TlsAlloc();
        EnterMutex emx(m_mx);
        m_instances.Add(this);
    }
    ~Tls()
    {
        TlsFree(m_slot);
        EnterMutex emx(m_mx);
        m_instances.Remove(this);
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

    void Free()
    {
    	T* p = (T*)TlsGetValue(m_slot);
        delete p;
    }

    static void FreeAll()
    {
        TlsFree(m_slot);
        EnterMutex emx(m_mx);
        for (int i=0; i<m_instances.GetSize(); i++)
        {
            m_instances[i]->Free();
        }

    }

private:
    DWORD m_slot;
    static Mutex m_mx;
    static List<Tls*> m_instances;

};



// Out-of-line definitions — one per class template, NOT per T
template <typename T>
Mutex Tls<T>::m_mx;

template <typename T>
List<Tls<T>*> Tls<T>::m_instances;


}

