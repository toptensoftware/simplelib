#include <thread>
#include <chrono>
#include <atomic>
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

Fact("AtomicSemaphore SpinWait Skips OnIdle When Signal Available")
{
	AtomicSemaphore s(1);
	int idleCalls = 0;

	Assert(s.SpinWait(0, 0,
		[](void* user) -> bool {
			(*(int*)user)++;
			return true;
		},
		&idleCalls));

	Assert(idleCalls == 0);		// signal was already there, never needed to idle
}

Fact("AtomicSemaphore SpinWait Calls OnIdle Before Sleeping")
{
	AtomicSemaphore s(0);
	std::atomic<int> idleCalls{0};
	bool waitResult = false;

	std::thread waiter([&]() {
		waitResult = s.SpinWait(10, 5000,
			[](void* user) -> bool {
				(*(std::atomic<int>*)user)++;
				return true;		// ok to go idle/sleep
			},
			&idleCalls);
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	Assert(idleCalls.load() >= 1);	// callback ran before the thread parked
	s.Release();

	waiter.join();
	Assert(waitResult);
}

Fact("AtomicSemaphore SpinWait OnIdle False Retries Without Sleeping")
{
	AtomicSemaphore s(0);
	int idleCalls = 0;

	bool result = s.SpinWait(0, 0,
		[](void* user) -> bool {
			int* calls = (int*)user;
			(*calls)++;
			return *calls >= 3;	// decline twice, then agree to idle
		},
		&idleCalls);

	Assert(!result);				// still no signal once it agreed to idle, and timeout is 0
	Assert(idleCalls == 3);		// retried until it agreed, not called forever
}

Fact("AtomicSemaphore Stop Unblocks Busy OnIdle Loop")
{
	AtomicSemaphore s(0);
	std::atomic<int> idleCalls{0};
	bool waitResult = true;	// start true so a bug that never runs the thread body wouldn't false-pass

	std::thread waiter([&]() {
		waitResult = s.SpinWait(0, 5000,
			[](void* user) -> bool {
				(*(std::atomic<int>*)user)++;
				return false;	// never agrees to idle - keeps retrying
			},
			&idleCalls);
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	s.Stop();

	waiter.join();
	Assert(!waitResult);
	Assert(idleCalls.load() > 0);	// callback was actually being driven, not skipped
}
