#include <atomic>
#include <chrono>
#include <thread>
#include "../UnitTesting.h"
#include "../Threading.h"
using namespace SimpleLib;

class FlagThread : public Thread
{
public:
	std::atomic<bool> ran{ false };

	virtual void ThreadProc() override
	{
		ran = true;
	}
};

class SleepThread : public Thread
{
public:
	int delayMs;
	std::atomic<bool> finished{ false };

	SleepThread(int delayMs) : delayMs(delayMs) {}

	virtual void ThreadProc() override
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
		finished = true;
	}
};

class CountingThread : public Thread
{
public:
	std::atomic<int>* counter;

	CountingThread(std::atomic<int>* counter) : counter(counter) {}

	virtual void ThreadProc() override
	{
		(*counter)++;
	}
};

class IdCapturingThread : public Thread
{
public:
	size_t idFromThreadProc = 0;
	std::atomic<bool> finished{ false };

	virtual void ThreadProc() override
	{
		idFromThreadProc = Thread::GetCurrentId();
		finished = true;
	}
};

Fact("Thread Runs ThreadProc")
{
	FlagThread t;
	t.Start();
	t.Join();
	Assert(t.ran);
}

Fact("Thread Destructor Without Start Is Safe")
{
	// Must not assert/crash even though ThreadProc is never run
	FlagThread t;
	Assert(!t.ran);
}

Fact("Thread Join Blocks Until Completion")
{
	SleepThread t(100);
	t.Start();
	t.Join();
	Assert(t.finished);	// Join() must not return before ThreadProc finishes
}

Fact("Thread Can Be Restarted After Join")
{
	std::atomic<int> counter{ 0 };
	CountingThread t(&counter);

	t.Start();
	t.Join();
	Assert(counter == 1);

	t.Start();
	t.Join();
	Assert(counter == 2);
}

Fact("Thread SetPriority Does Not Crash")
{
	SleepThread t(50);
	t.Start();
	t.SetPriority(ThreadPriority::BelowNormal);
	t.SetPriority(ThreadPriority::Normal);
	t.SetPriority(ThreadPriority::AboveNormal);
	t.SetPriority(ThreadPriority::RealTime);
	t.Join();
	Assert(t.finished);
}

Fact("Thread SetDescription Does Not Crash")
{
	SleepThread t(20);
	t.Start();
	t.SetDescription("SimpleLib Test Thread");
	t.Join();
	Assert(t.finished);
}

Fact("Thread GetId Matches Current Id Seen From ThreadProc")
{
	IdCapturingThread t;
	t.Start();
	t.Join();

	Assert(t.finished);
	Assert(t.idFromThreadProc != 0);
	Assert(t.GetId() == t.idFromThreadProc);
}

Fact("Thread GetCurrentId Differs Between Threads")
{
	size_t mainId = Thread::GetCurrentId();

	IdCapturingThread t;
	t.Start();
	t.Join();

	Assert(t.idFromThreadProc != mainId);
}

Fact("Thread GetCurrentHandle Is Not Null")
{
	Assert(Thread::GetCurrentHandle() != nullptr);
}

class SelfPriorityThread : public Thread
{
public:
	std::atomic<bool> finished{ false };
	std::atomic<bool> handleWasValid{ false };

	virtual void ThreadProc() override
	{
		// Exercises the Start() race: without starting suspended, m_handle
		// may not be stored yet when this runs on the new thread. Can't use
		// Assert() here since it throws and nothing on this thread catches it
		// -- capture the result and check it back on the main thread instead.
		handleWasValid = GetHandle() != nullptr;
		SetPriority(ThreadPriority::AboveNormal);
		SetDescription("Self Priority Thread");
		finished = true;
	}
};

Fact("Thread Can Set Its Own Priority And Description From ThreadProc")
{
	for (int i = 0; i < 50; i++)
	{
		SelfPriorityThread t;
		t.Start();
		t.Join();
		Assert(t.finished);
		Assert(t.handleWasValid);
	}
}

Fact("Thread Multiple Threads Run Concurrently")
{
	std::atomic<int> counter{ 0 };
	const int kThreads = 8;

	CountingThread* threads[kThreads];
	for (int i = 0; i < kThreads; i++)
		threads[i] = new CountingThread(&counter);

	for (int i = 0; i < kThreads; i++)
		threads[i]->Start();
	for (int i = 0; i < kThreads; i++)
		threads[i]->Join();

	Assert(counter == kThreads);

	for (int i = 0; i < kThreads; i++)
		delete threads[i];
}
