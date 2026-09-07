#pragma once

#include "../Platform/Platform.h"

namespace SimpleLib
{

class Mutex
{
public:
	Mutex()
	{
		Platform::mutexCreate(m_mutex);
	}

	virtual ~Mutex()
	{
		Platform::mutexDestroy(m_mutex);
	}

	void Enter()
	{
		Platform::mutexEnter(m_mutex);
	}
	bool TryEnter()
	{
		return Platform::mutexTryEnter(m_mutex);
	}
	void Leave()
	{
		return Platform::mutexLeave(m_mutex);
	}
	bool IsHeld()
	{
		return Platform::mutexIsHeld(m_mutex);
	}

private:
	Platform::TMutex m_mutex;
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