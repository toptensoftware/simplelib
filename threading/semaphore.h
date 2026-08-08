#pragma once

#include "common.h"

namespace SimpleLib
{

class Semaphore
{
public:
	Semaphore(int initialValue)
	{
        m_handle = CreateSemaphore(nullptr, initialValue, 0x7FFFFFFF, nullptr);
	}
	~Semaphore()
	{
        CloseHandle(m_handle);
	}

	void Init(int initialValue)
	{
        CloseHandle(m_handle);
        m_handle = CreateSemaphore(nullptr, initialValue, 0x7FFFFFFF, nullptr);
	}

	bool Wait(uint32_t timeout = kWaitForever)
	{
        return WaitForSingleObject(m_handle, timeout) == WAIT_OBJECT_0;
	}

	void Release(int count = 1)
	{
	    ReleaseSemaphore(m_handle, count, nullptr);
	}

	HANDLE m_handle;
};



}