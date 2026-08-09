#pragma once

#include "../Core/String.h"
#include "../Core/Encoding.h"
#include "ThreadLocal.h"

namespace SimpleLib
{

// Thread priority
enum class ThreadPriority
{
	BelowNormal,
	Normal,
	AboveNormal,
	RealTime,
};

class Thread
{
public:
	Thread(ThreadPriority priority = ThreadPriority::Normal, const char* description = nullptr)
	{
		m_priority = priority;
		m_description = description;
	}

	virtual ~Thread()
	{
		assert(m_handle == nullptr);
        if (m_handle)
            CloseHandle(m_handle);
	}

	void Start()
	{
		assert(m_handle == nullptr);

        DWORD dwID;
        m_handle = CreateThread(nullptr, 0, &ThreadProcStub, this, 0, &dwID);

		SetPriority(m_priority);
		if (!m_description.IsEmpty())
			SetDescription(m_description);
	}

	void Wait(uint32_t timeout = kWaitForever)
	{
		if (m_handle)
		{
            WaitForSingleObject(m_handle, timeout);
            CloseHandle(m_handle);
            m_handle = nullptr;
		}
	}

	void SetDescription(const char* psz)
	{
		assert(m_handle != nullptr);
		SetThreadDescription(m_handle, Encode<wchar_t>(psz).sz());
	}

	void SetPriority(ThreadPriority priority)
	{
		assert(m_handle != nullptr);
		switch (priority)
		{
			case ThreadPriority::RealTime:
				// High priority
				SetThreadPriority(m_handle, THREAD_PRIORITY_TIME_CRITICAL);
				break;

			case ThreadPriority::Normal:
				SetThreadPriority(m_handle, THREAD_PRIORITY_NORMAL);
				break;

			case ThreadPriority::AboveNormal:
				SetThreadPriority(m_handle, THREAD_PRIORITY_ABOVE_NORMAL);
				break;

			case ThreadPriority::BelowNormal:
				SetThreadPriority(m_handle, THREAD_PRIORITY_BELOW_NORMAL);
				break;
		}

	}

protected:
	virtual void ThreadProc()=0;

private:
	void* m_handle = nullptr;
	ThreadPriority m_priority;
	String<char> m_description;

	void ThreadProcEntry()
	{
		ThreadProc();
		ThreadLocalBase::FreeAll();
	}

	static DWORD WINAPI ThreadProcStub(void* param)
	{
		((Thread*)param)->ThreadProcEntry();
		return 0;
	}

};

}