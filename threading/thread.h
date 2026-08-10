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

        DWORD dwId;
        m_handle = CreateThread(nullptr, 0, &ThreadProcStub, this, 0, &dwId);
        m_id = dwId;

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

	void* GetHandle()
	{
		return m_handle;
	}

	// Id of this thread instance
	size_t GetId()
	{
		return m_id;
	}

	// Id of the calling thread
	static size_t GetCurrentId()
	{
		return (size_t)GetCurrentThreadId();
	}

	// Pseudo-handle of the calling thread. Only valid for use by that same
	// thread (eg: passing to SetThreadPriority) - it can't be used from
	// another thread to wait on or otherwise reference this thread.
	static void* GetCurrentHandle()
	{
		return (void*)GetCurrentThread();
	}

protected:
	virtual void ThreadProc()=0;

private:
	void* m_handle = nullptr;
	size_t m_id = 0;
	ThreadPriority m_priority;
	String m_description;

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