#pragma once

#include "../Platform/Platform.h"

namespace SimpleLib
{

class Semaphore
{
public:
	Semaphore(int initialValue)
	{
		Platform::semaCreate(m_sema, initialValue);
	}
	~Semaphore()
	{
		Platform::semaDestroy(m_sema);
	}

	void Init(int initialValue)
	{
		Platform::semaDestroy(m_sema);
		Platform::semaCreate(m_sema, initialValue);
	}

	bool Wait(uint32_t timeout = kWaitForever)
	{
		return Platform::semaWait(m_sema, timeout);
	}

	void Release(int count = 1)
	{
		Platform::semaRelease(m_sema, count);
	}

	Platform::TSema m_sema;
};



}