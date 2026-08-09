#include <thread>
#include <chrono>
#include "../UnitTesting.h"
#include "../Threading.h"
using namespace SimpleLib;

Fact("Atomic Default Constructor")
{
	Atomic<uint32_t> a;
	Assert(a.Get() == 0);
}

Fact("Atomic Get Set")
{
	Atomic<uint32_t> a(5);
	Assert(a.Get() == 5);

	uint32_t old = a.Set(10);
	Assert(old == 5);
	Assert(a.Get() == 10);
}

Fact("Atomic TrySet")
{
	Atomic<uint32_t> a(5);

	// Compare matches current value -> succeeds
	uint32_t prev = a.TrySet(10, 5);
	Assert(prev == 5);
	Assert(a.Get() == 10);

	// Compare no longer matches -> fails, returns actual current value
	prev = a.TrySet(20, 5);
	Assert(prev == 10);
	Assert(a.Get() == 10);
}

Fact("Atomic Inc Dec")
{
	Atomic<uint32_t> a(5);
	Assert(a.Inc() == 6);
	Assert(a.Get() == 6);
	Assert(a.Dec() == 5);
	Assert(a.Get() == 5);
}

Fact("Atomic Add FetchAdd")
{
	Atomic<uint32_t> a(5);

	Assert(a.Add(3) == 8);		// returns new value
	Assert(a.Get() == 8);

	uint32_t old = a.FetchAdd(2);
	Assert(old == 8);			// returns old value
	Assert(a.Get() == 10);
}

Fact("Atomic Pointer Sized")
{
	int x = 1, y = 2, z = 3;
	Atomic<int*> p(&x);
	Assert(p.Get() == &x);

	int* old = p.Set(&y);
	Assert(old == &x);
	Assert(p.Get() == &y);

	int* prev = p.TrySet(&z, &y);
	Assert(prev == &y);
	Assert(p.Get() == &z);

	prev = p.TrySet(&x, &y);	// compare no longer matches
	Assert(prev == &z);
	Assert(p.Get() == &z);
}

Fact("Atomic Size_t Sized")
{
	Atomic<size_t> a((size_t)100);
	Assert(a.Get() == 100);
	Assert(a.Inc() == 101);
	Assert(a.Add(10) == 111);
}

Fact("Atomic Wait Times Out When Value Unchanged")
{
	Atomic<uint32_t> a(0);
	Assert(!a.Wait(0, 50));
}

Fact("Atomic Wait Returns Immediately If Value Already Changed")
{
	Atomic<uint32_t> a(1);
	Assert(a.Wait(0, 50));		// current value (1) != expected (0), returns true immediately
}

Fact("Atomic Wait WakeOne")
{
	Atomic<uint32_t> a(0);

	std::thread waiter([&]() {
		a.Wait(0, 5000);
	});

	// Give the waiter time to actually start waiting before signalling
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	a.Set(1);
	a.WakeOne();

	waiter.join();
	Assert(a.Get() == 1);
}

Fact("Atomic Wait WakeAll Wakes Multiple Waiters")
{
	Atomic<uint32_t> a(0);
	std::atomic<int> woken(0);

	std::thread waiters[4];
	for (auto& t : waiters)
	{
		t = std::thread([&]() {
			if (a.Wait(0, 5000))
				woken++;
		});
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	a.Set(1);
	a.WakeAll();

	for (auto& t : waiters)
		t.join();

	Assert(woken == 4);
}
