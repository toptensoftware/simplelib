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

Fact("Thread Runs ThreadProc")
{
	FlagThread t;
	t.Start();
	t.Wait();
	Assert(t.ran);
}

Fact("Thread Destructor Without Start Is Safe")
{
	// Must not assert/crash even though ThreadProc is never run
	FlagThread t;
	Assert(!t.ran);
}

Fact("Thread Wait Blocks Until Completion")
{
	SleepThread t(100);
	t.Start();
	t.Wait();
	Assert(t.finished);	// Wait() must not return before ThreadProc finishes
}

Fact("Thread Can Be Restarted After Wait")
{
	std::atomic<int> counter{ 0 };
	CountingThread t(&counter);

	t.Start();
	t.Wait();
	Assert(counter == 1);

	t.Start();
	t.Wait();
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
	t.Wait();
	Assert(t.finished);
}

Fact("Thread SetDescription Does Not Crash")
{
	SleepThread t(20);
	t.Start();
	t.SetDescription("SimpleLib Test Thread");
	t.Wait();
	Assert(t.finished);
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
		threads[i]->Wait();

	Assert(counter == kThreads);

	for (int i = 0; i < kThreads; i++)
		delete threads[i];
}
