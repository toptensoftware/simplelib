#include <thread>
#include <chrono>
#include "../UnitTesting.h"
#include "../Threading.h"
using namespace SimpleLib;

Fact("Semaphore Wait Consumes A Signal")
{
	Semaphore s(1);
	Assert(s.Wait(0));		// initial count of 1 is immediately available
	Assert(!s.Wait(0));	// now empty, non-blocking wait times out
}

Fact("Semaphore Release Adds Signals")
{
	Semaphore s(0);
	s.Release(3);

	Assert(s.Wait(0));
	Assert(s.Wait(0));
	Assert(s.Wait(0));
	Assert(!s.Wait(0));
}

Fact("Semaphore Init Reinitializes")
{
	Semaphore s(0);
	Assert(!s.Wait(0));

	s.Init(2);
	Assert(s.Wait(0));
	Assert(s.Wait(0));
	Assert(!s.Wait(0));
}

Fact("Semaphore Producer Consumer")
{
	Semaphore s(0);
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

Fact("Semaphore Counts Multiple Producers")
{
	Semaphore s(0);
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
