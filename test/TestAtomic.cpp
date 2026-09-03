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

Fact("Atomic CompareExchange")
{
	Atomic<uint32_t> a(5);

	// Compare matches current value -> succeeds
	uint32_t prev = a.CompareExchange(10, 5);
	Assert(prev == 5);
	Assert(a.Get() == 10);

	// Compare no longer matches -> fails, returns actual current value
	prev = a.CompareExchange(20, 5);
	Assert(prev == 10);
	Assert(a.Get() == 10);
}

Fact("Atomic TrySet")
{
	Atomic<uint32_t> a(5);

	// Compare matches current value -> succeeds
	Assert(a.TrySet(10, 5) == true);
	Assert(a.Get() == 10);

	// Compare no longer matches -> fails, value unchanged
	Assert(a.TrySet(20, 5) == false);
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

	int* prev = p.CompareExchange(&z, &y);
	Assert(prev == &y);
	Assert(p.Get() == &z);

	prev = p.CompareExchange(&x, &y);	// compare no longer matches
	Assert(prev == &z);
	Assert(p.Get() == &z);

	Assert(p.TrySet(&x, &z) == true);
	Assert(p.Get() == &x);

	Assert(p.TrySet(&y, &z) == false);	// compare no longer matches
	Assert(p.Get() == &x);
}

Fact("Atomic Size_t Sized")
{
	Atomic<size_t> a((size_t)100);
	Assert(a.Get() == 100);
	Assert(a.Inc() == 101);
	Assert(a.Add(10) == 111);
}

Fact("Atomic Float Default Constructor")
{
	Atomic<float> a;
	Assert(a.Get() == 0.0f);
}

Fact("Atomic Float Get Set")
{
	Atomic<float> a(5.5f);
	Assert(a.Get() == 5.5f);

	float old = a.Set(10.25f);
	Assert(old == 5.5f);
	Assert(a.Get() == 10.25f);
}

Fact("Atomic Float CompareExchange")
{
	Atomic<float> a(5.5f);

	// Compare matches current value -> succeeds
	float prev = a.CompareExchange(10.25f, 5.5f);
	Assert(prev == 5.5f);
	Assert(a.Get() == 10.25f);

	// Compare no longer matches -> fails, returns actual current value
	prev = a.CompareExchange(20.0f, 5.5f);
	Assert(prev == 10.25f);
	Assert(a.Get() == 10.25f);
}

Fact("Atomic Float TrySet")
{
	Atomic<float> a(5.5f);

	// Compare matches current value -> succeeds
	Assert(a.TrySet(10.25f, 5.5f) == true);
	Assert(a.Get() == 10.25f);

	// Compare no longer matches -> fails, value unchanged
	Assert(a.TrySet(20.0f, 5.5f) == false);
	Assert(a.Get() == 10.25f);
}

Fact("Atomic Float Round Trips Bit Pattern Exactly")
{
	// Guards against the naive `(T)atomicLoad(...)` cast, which would do a
	// numeric conversion (bits reinterpreted as an integer, then converted
	// to float) rather than a bit-cast.
	Atomic<float> a(0.0f);
	a.Set(1.0f);						// bit pattern 0x3f800000
	Assert(a.Get() == 1.0f);

	a.Set(-1.0f);
	Assert(a.Get() == -1.0f);

	a.Set(0.1f);
	Assert(a.Get() == 0.1f);
}

Fact("Atomic Float Wait Times Out When Value Unchanged")
{
	Atomic<float> a(0.0f);
	Assert(!a.Wait(0.0f, 50));
}

Fact("Atomic Float Wait WakeOne")
{
	Atomic<float> a(0.0f);

	std::thread waiter([&]() {
		a.Wait(0.0f, 5000);
	});

	// Give the waiter time to actually start waiting before signalling
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	a.Set(1.5f);
	a.WakeOne();

	waiter.join();
	Assert(a.Get() == 1.5f);
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

Fact("Atomic Bool Get Set")
{
	Atomic<bool> a;
	Assert(a.Get() == false);

	Atomic<bool> b(true);
	Assert(b.Get() == true);

	bool old = b.Set(false);
	Assert(old == true);
	Assert(b.Get() == false);
}

Fact("Atomic Bool CompareExchange TrySet")
{
	Atomic<bool> a(false);

	Assert(a.CompareExchange(true, false) == false);
	Assert(a.Get() == true);

	Assert(a.CompareExchange(false, false) == true);	// compare no longer matches
	Assert(a.Get() == true);

	Assert(a.TrySet(false, true) == true);
	Assert(a.Get() == false);
	Assert(a.TrySet(true, true) == false);
	Assert(a.Get() == false);
}

Fact("Atomic Bool Wait WakeOne")
{
	Atomic<bool> a(false);

	std::thread waiter([&]() {
		a.Wait(false, 5000);
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	a.Set(true);
	a.WakeOne();

	waiter.join();
	Assert(a.Get() == true);
}

Fact("Atomic Bool Wait Times Out When Value Unchanged")
{
	Atomic<bool> a(false);
	Assert(!a.Wait(false, 50));
}

Fact("Atomic Int16 Get Set CompareExchange")
{
	Atomic<int16_t> a((int16_t)5);
	Assert(a.Get() == 5);

	int16_t old = a.Set((int16_t)-3);
	Assert(old == 5);
	Assert(a.Get() == -3);							// sign preserved through the packed word

	Assert(a.CompareExchange((int16_t)100, (int16_t)-3) == -3);
	Assert(a.Get() == 100);
}

Fact("Atomic Int16 Inc Dec Add Wraps Like The Narrow Type")
{
	Atomic<int16_t> a((int16_t)5);
	Assert(a.Inc() == 6);
	Assert(a.Get() == 6);
	Assert(a.Dec() == 5);
	Assert(a.Add((int16_t)10) == 15);
	Assert(a.FetchAdd((int16_t)2) == 15);
	Assert(a.Get() == 17);

	Atomic<uint8_t> b((uint8_t)254);
	Assert(b.Inc() == 255);
	Assert(b.Inc() == 0);							// wraps at the type width, storage stays normalized
	Assert(b.Get() == 0);
	Assert(b.CompareExchange((uint8_t)1, (uint8_t)0) == 0);	// no stale high bits left behind
	Assert(b.Get() == 1);
}

Fact("Atomic Int16 Concurrent FetchAdd Never Loses An Update")
{
	Atomic<int16_t> a((int16_t)0);

	const int kThreads = 4;
	const int kPerThread = 2000;			// 8000 total fits in int16_t

	std::thread threads[kThreads];
	for (auto& t : threads)
	{
		t = std::thread([&]() {
			for (int i = 0; i < kPerThread; i++)
				a.FetchAdd((int16_t)1);
		});
	}
	for (auto& t : threads)
		t.join();

	Assert(a.Get() == kThreads * kPerThread);
}

Fact("AtomicTaggedPtr Get Reflects Initial State")
{
	AtomicTaggedPtr<int> p;
	auto v = p.Get();
	Assert(v.ptr == nullptr);
	Assert(v.tag == 0);
}

Fact("AtomicTaggedPtr TrySet Succeeds When Value Matches")
{
	AtomicTaggedPtr<int> p;
	int x = 1;

	auto old = p.Get();
	Assert(p.TrySet({ &x, old.tag + 1 }, old));

	auto now = p.Get();
	Assert(now.ptr == &x);
	Assert(now.tag == old.tag + 1);
}

Fact("AtomicTaggedPtr TrySet Fails When Pointer Differs")
{
	AtomicTaggedPtr<int> p;
	int x = 1, y = 2;

	AtomicTaggedPtr<int>::Value stale{ &x, 0 };	// never actually set
	Assert(!p.TrySet({ &y, 1 }, stale));
	Assert(p.Get().ptr == nullptr);	// unchanged
}

Fact("AtomicTaggedPtr TrySet Fails On Stale Tag Even When Pointer Matches")
{
	// This is the whole point of the tag: after the pointer cycles back to
	// a value a delayed thread already saw (pop then re-push, same
	// address), a compare using the OLD tag must not match the new state,
	// even though the pointer half is identical - this is exactly what
	// defeats the ABA race that plain pointer CAS can't.
	AtomicTaggedPtr<int> p;
	int x = 1;

	auto gen0 = p.Get();							// {nullptr, 0}
	Assert(p.TrySet({ &x, gen0.tag + 1 }, gen0));	// -> {&x, 1}

	auto gen1 = p.Get();
	Assert(p.TrySet({ nullptr, gen1.tag + 1 }, gen1));	// -> {nullptr, 2}

	auto gen2 = p.Get();
	Assert(p.TrySet({ &x, gen2.tag + 1 }, gen2));	// -> {&x, 3} - pointer cycled back to &x

	// A thread holding the STALE {&x, 1} snapshot from earlier must fail,
	// even though the pointer half matches the current value
	Assert(!p.TrySet({ nullptr, 99 }, gen0));
	Assert(p.Get().ptr == &x);
	Assert(p.Get().tag == 3);
}

Fact("AtomicTaggedPtr Set Is Non Atomic Overwrite")
{
	AtomicTaggedPtr<int> p;
	int x = 1;
	p.Set({ &x, 5 });

	auto v = p.Get();
	Assert(v.ptr == &x);
	Assert(v.tag == 5);
}

Fact("AtomicTaggedPtr Concurrent TrySet Never Loses An Update")
{
	// Many threads race to increment the tag from a known starting value;
	// exactly one TrySet should succeed per generation, so after N total
	// successful attempts the tag must have advanced by exactly N with no
	// lost updates.
	AtomicTaggedPtr<int> p;
	int x = 1;
	p.Set({ &x, 0 });

	const int kThreads = 8;
	const int kIncrementsPerThread = 20000;
	std::atomic<int> totalSucceeded{ 0 };

	std::thread threads[kThreads];
	for (int t = 0; t < kThreads; t++)
	{
		threads[t] = std::thread([&]() {
			int done = 0;
			while (done < kIncrementsPerThread)
			{
				auto cur = p.Get();
				if (p.TrySet({ &x, cur.tag + 1 }, cur))
				{
					done++;
					totalSucceeded++;
				}
			}
		});
	}
	for (auto& th : threads)
		th.join();

	Assert(totalSucceeded == kThreads * kIncrementsPerThread);
	Assert(p.Get().tag == (uint64_t)(kThreads * kIncrementsPerThread));
	Assert(p.Get().ptr == &x);
}
