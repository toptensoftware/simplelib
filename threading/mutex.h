#pragma once

namespace SimpleLib
{

class Mutex
{
public:
	Mutex()
	{
        InitializeCriticalSection(&m_cs);
        SetCriticalSectionSpinCount(&m_cs, 4000);
	}

	virtual ~Mutex()
	{
        DeleteCriticalSection(&m_cs);
	}

	void Enter()
	{
        EnterCriticalSection(&m_cs);
	}
	bool TryEnter()
	{
        return TryEnterCriticalSection(&m_cs);
	}
	void Leave()
	{
        LeaveCriticalSection(&m_cs);
	}
	bool IsHeld()
	{
    	return m_cs.OwningThread == (HANDLE)(size_t)GetCurrentThreadId();
	}

private:
	CRITICAL_SECTION m_cs;
};

class EnterMutex
{
public:
	EnterMutex()
	{
		m_pMutex = nullptr;
	}

	EnterMutex(Mutex& mutex)
	{
		m_pMutex = nullptr;
		Enter(mutex);
	}

	~EnterMutex()
	{
		if (m_pMutex != nullptr)
			Leave();
	}

	void Enter(Mutex& mutex)
	{
		assert(m_pMutex == nullptr);
		m_pMutex = &mutex;
		m_pMutex->Enter();
	}

	void Leave()
	{
		assert(m_pMutex != nullptr);
		m_pMutex->Leave();
		m_pMutex = nullptr;
	}

	Mutex* m_pMutex;
};




}