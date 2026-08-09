#include <thread>
#include <chrono>
#include "../UnitTesting.h"
#include "../Threading.h"
using namespace SimpleLib;

Fact("AtomicSemaphore Wait Consumes A Signal")
{
	AtomicSemaphore s(1);
	Assert(s.Wait(0));
	Assert(!s.Wait(0));		// no signals left, non-blocking wait times out
}

Fact("AtomicSemaphore Release Adds Signals")
{
	AtomicSemaphore s(0);
	s.Release(3);

	Assert(s.Wait(0));
	Assert(s.Wait(0));
	Assert(s.Wait(0));
	Assert(!s.Wait(0));
}

Fact("AtomicSemaphore SpinWait Consumes Available Signal")
{
	AtomicSemaphore s(1);
	Assert(s.SpinWait(1000, 0));
	Assert(!s.SpinWait(1000, 0));
}

Fact("AtomicSemaphore Producer Consumer")
{
	AtomicSemaphore s(0);
	bool consumed = false;

	std::thread consumer([&]() {
		if (s.Wait(5000))
			consumed = true;
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	s.Release();

	consumer.join();
	Assert(consumed);
}

Fact("AtomicSemaphore Counts Multiple Releases From Multiple Threads")
{
	AtomicSemaphore s(0);
	const int kThreads = 8;

	std::thread threads[kThreads];
	for (auto& t : threads)
		t = std::thread([&]() { s.Release(); });
	for (auto& t : threads)
		t.join();

	for (int i = 0; i < kThreads; i++)
		Assert(s.Wait(0));
	Assert(!s.Wait(0));
}

Fact("AtomicSemaphore Stop Unblocks Waiter")
{
	AtomicSemaphore s(0);
	bool waitResult = true;	// start true so a bug that never runs the thread body wouldn't false-pass

	std::thread waiter([&]() {
		waitResult = s.Wait(5000);
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	s.Stop();

	waiter.join();
	Assert(!waitResult);
}

Fact("AtomicSemaphore Reset Reopens After Stop")
{
	AtomicSemaphore s(0);

	std::thread waiter([&]() {
		s.Wait(5000);
	});
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	s.Stop();
	waiter.join();

	s.Reset();
	s.Release(1);
	Assert(s.Wait(0));
}
