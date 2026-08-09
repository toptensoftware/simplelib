#include <thread>
#include <chrono>
#include "../UnitTesting.h"
#include "../Threading.h"
using namespace SimpleLib;

Fact("Mutex Enter Leave")
{
	Mutex m;
	m.Enter();
	Assert(m.IsHeld());
	m.Leave();
	Assert(!m.IsHeld());
}

Fact("Mutex TryEnter Succeeds When Free")
{
	Mutex m;
	Assert(m.TryEnter());
	Assert(m.IsHeld());
	m.Leave();
}

Fact("Mutex TryEnter Fails While Held By Another Thread")
{
	Mutex m;
	m.Enter();

	std::thread t([&]() {
		Assert(!m.TryEnter());
	});
	t.join();

	m.Leave();

	std::thread t2([&]() {
		Assert(m.TryEnter());
		m.Leave();
	});
	t2.join();
}

Fact("Mutex EnterMutex RAII")
{
	Mutex m;
	{
		EnterMutex em(m);
		Assert(m.IsHeld());
	}
	Assert(!m.IsHeld());
}

Fact("Mutex EnterMutex Default Constructor Then Enter")
{
	Mutex m;
	EnterMutex em;
	em.Enter(m);
	Assert(m.IsHeld());
	em.Leave();
	Assert(!m.IsHeld());
}

Fact("Mutex Protects Shared Counter Under Contention")
{
	Mutex m;
	int counter = 0;
	const int kThreads = 8;
	const int kIncrements = 2000;

	std::thread threads[kThreads];
	for (auto& t : threads)
	{
		t = std::thread([&]() {
			for (int i = 0; i < kIncrements; i++)
			{
				EnterMutex em(m);
				counter++;
			}
		});
	}
	for (auto& t : threads)
		t.join();

	Assert(counter == kThreads * kIncrements);
}
