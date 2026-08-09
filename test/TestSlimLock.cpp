#include <thread>
#include <chrono>
#include <atomic>
#include "../UnitTesting.h"
#include "../Threading.h"
using namespace SimpleLib;

Fact("SlimLock TryEnterExclusive Succeeds When Free")
{
	SlimLock lock;
	Assert(lock.TryEnterExclusive());
	lock.LeaveExclusive();
}

Fact("SlimLock TryEnterExclusive Fails While Held")
{
	SlimLock lock;
	lock.EnterExclusive();
	Assert(!lock.TryEnterExclusive());	// TryEnter never blocks, safe even though SRWLOCK isn't recursive
	lock.LeaveExclusive();
}

Fact("SlimLock TryEnterShared Succeeds When Free")
{
	SlimLock lock;
	Assert(lock.TryEnterShared());
	lock.LeaveShared();
}

Fact("SlimLock Generic Enter Leave With Bool")
{
	SlimLock lock;
	lock.Enter(true);
	Assert(!lock.TryEnterShared());
	lock.Leave(true);

	lock.Enter(false);
	Assert(lock.TryEnterShared());		// shared allows concurrent shared holders
	lock.LeaveShared();
	lock.Leave(false);
}

Fact("SlimLock EnterSlimLock RAII")
{
	SlimLock lock;
	{
		EnterSlimLock esl(lock, true);
		Assert(!lock.TryEnterExclusive());
	}
	Assert(lock.TryEnterExclusive());
	lock.LeaveExclusive();
}

Fact("SlimLock Exclusive Excludes Other Thread")
{
	SlimLock lock;
	lock.EnterExclusive();

	std::thread t([&]() {
		Assert(!lock.TryEnterExclusive());
		Assert(!lock.TryEnterShared());
	});
	t.join();

	lock.LeaveExclusive();

	std::thread t2([&]() {
		Assert(lock.TryEnterExclusive());
		lock.LeaveExclusive();
	});
	t2.join();
}

Fact("SlimLock Shared Allows Concurrent Readers From Other Thread")
{
	SlimLock lock;
	lock.EnterShared();

	std::thread t([&]() {
		Assert(lock.TryEnterShared());
		lock.LeaveShared();
	});
	t.join();

	lock.LeaveShared();
}

Fact("SlimLock Protects Shared Counter Under Contention")
{
	SlimLock lock;
	int counter = 0;
	const int kThreads = 8;
	const int kIncrements = 2000;

	std::thread threads[kThreads];
	for (auto& t : threads)
	{
		t = std::thread([&]() {
			for (int i = 0; i < kIncrements; i++)
			{
				EnterSlimLock esl(lock, true);
				counter++;
			}
		});
	}
	for (auto& t : threads)
		t.join();

	Assert(counter == kThreads * kIncrements);
}
