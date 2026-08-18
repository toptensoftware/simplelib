#pragma once

#include "../Core/String.h"
#include "../Core/Encoding.h"
#include "ThreadLocal.h"

namespace SimpleLib
{

class Thread
{
public:
	Thread(ThreadPriority priority = ThreadPriority::Normal, const char* description = nullptr)
	{
		m_priority = priority;
		m_description = description;
		Platform::threadInit(m_thread);
	}

	virtual ~Thread()
	{
		Platform::threadDestroy(m_thread);
	}

	bool Start()
	{
		assert(!Platform::threadIsCreated(m_thread));

		// Star the thread
		m_id = Platform::threadStart(m_thread, &ThreadProcStub, this);
		if (m_id == 0)
			return false;

		// Setup attributes
		if (m_priority != ThreadPriority::Normal)
			SetPriority(m_priority);
		if (!m_description.IsEmpty())
			SetDescription(m_description);

		// Resume it
		Platform::threadResume(m_thread);

		return true;
	}

	bool Join(uint32_t timeout = kWaitForever)
	{
		return Platform::threadJoin(m_thread, timeout);
	}

	void SetDescription(const char* psz)
	{
		Platform::threadSetDescription(m_thread, psz);
	}

	void SetPriority(ThreadPriority priority)
	{
		Platform::threadSetPriority(m_thread, priority);
	}

	Platform::TThread GetHandle()
	{
		return m_thread;
	}

	// Id of this thread instance
	size_t GetId()
	{
		return m_id;
	}

	// Id of the calling thread
	static size_t GetCurrentId()
	{
		return Platform::threadCurrentId();
	}

	// Pseudo-handle of the calling thread. Only valid for use by that same
	// thread (eg: passing to SetThreadPriority) - it can't be used from
	// another thread to wait on or otherwise reference this thread.
	static Platform::TThread GetCurrentHandle()
	{
		return Platform::threadCurrentHandle();
	}

	static void Yield()
	{
		Platform::Yield();
	}

	static void Sleep(uint32_t ms)
	{
		Platform::Sleep(ms);
	}

protected:
	virtual void ThreadProc()=0;

private:
	Platform::TThread m_thread;
	size_t m_id = 0;
	ThreadPriority m_priority;
	String m_description;

	void ThreadProcEntry()
	{
		ThreadProc();
		ThreadLocalBase::FreeAll();
	}

	static void ThreadProcStub(void* param)
	{
		((Thread*)param)->ThreadProcEntry();
	}

};

}