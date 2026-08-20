#include <thread>
#include <atomic>
#include <vector>
#include "../UnitTesting.h"
#include "../Threading.h"
using namespace SimpleLib;

// A default-constructible element that tracks live instances, so we can
// verify Reset()/RemoveAll() correctly destroy elements still sitting in
// the queue rather than just abandoning their storage.
class TrackedValue
{
public:
	TrackedValue(int val = 0) : Value(val) { s_iInstances++; }
	TrackedValue(const TrackedValue& other) : Value(other.Value) { s_iInstances++; }
	~TrackedValue() { s_iInstances--; }

	int Value;
	inline static int s_iInstances = 0;
};

Fact("MpmcQueue Basic Write Read")
{
	MpmcQueue<int> q(8);
	Assert(q.IsLikelyEmpty());

	Assert(q.TryWrite(1));
	Assert(q.TryWrite(2));
	Assert(!q.IsLikelyEmpty());
	Assert(q.GetLikelyCount() == 2);

	int val;
	Assert(q.Read(val));
	Assert(val == 1);
	Assert(q.Read(val));
	Assert(val == 2);
	Assert(q.IsLikelyEmpty());
	Assert(!q.Read(val));
}

Fact("MpmcQueue MustWrite")
{
	MpmcQueue<int> q(8);
	q.MustWrite(10);
	q.MustWrite(20);

	int val;
	Assert(q.Read(val) && val == 10);
	Assert(q.Read(val) && val == 20);
}

Fact("MpmcQueue Fills To Full Capacity")
{
	MpmcQueue<int> q(4);
	Assert(q.GetCapacity() == 4);

	int count = 0;
	while (q.TryWrite(count))
		count++;

	Assert(count == q.GetCapacity());	// no sentinel slot wasted, unlike SpscQueue
	Assert(q.IsLikelyFull());
	Assert(!q.TryWrite(999));
}

Fact("MpmcQueue TryWrite Succeeds Again After Read Frees A Slot")
{
	MpmcQueue<int> q(4);
	while (q.TryWrite(0)) {}
	Assert(q.IsLikelyFull());

	int val;
	Assert(q.Read(val));
	Assert(!q.IsLikelyFull());
	Assert(q.TryWrite(999));
	Assert(q.IsLikelyFull());
}

Fact("MpmcQueue Reset")
{
	MpmcQueue<int> q(4);
	q.TryWrite(1);
	q.TryWrite(2);

	q.Reset(8);
	Assert(q.IsLikelyEmpty());
	Assert(q.GetCapacity() == 8);

	Assert(q.TryWrite(5));
	Assert(q.GetLikelyCount() == 1);
}

Fact("MpmcQueue Reset Destroys Pending Elements")
{
	TrackedValue::s_iInstances = 0;
	{
		MpmcQueue<TrackedValue> q(8);
		q.TryWrite(TrackedValue(1));
		q.TryWrite(TrackedValue(2));
		q.TryWrite(TrackedValue(3));
		Assert(TrackedValue::s_iInstances == 3);

		q.Reset(16);
		Assert(TrackedValue::s_iInstances == 0);

		q.TryWrite(TrackedValue(4));
		Assert(TrackedValue::s_iInstances == 1);
	}
	Assert(TrackedValue::s_iInstances == 0);
}

Fact("MpmcQueue Wraparound Preserves FIFO Order")
{
	MpmcQueue<int> q(4);	// small buffer forces frequent wraparound

	int nextWrite = 0;
	int nextRead = 0;
	int val;

	while (nextRead < 1000)
	{
		if (nextWrite < 1000 && q.TryWrite(nextWrite))
			nextWrite++;

		if (q.Read(val))
		{
			Assert(val == nextRead);
			nextRead++;
		}
	}

	Assert(q.IsLikelyEmpty());
}

Fact("MpmcQueue Single Producer Single Consumer Threads")
{
	MpmcQueue<int> q(64);
	const int kItemCount = 200000;

	std::thread producer([&]() {
		int i = 0;
		while (i < kItemCount)
		{
			if (q.TryWrite(i))
				i++;
		}
	});

	std::thread consumer([&]() {
		int expected = 0;
		int val;
		while (expected < kItemCount)
		{
			if (q.Read(val))
			{
				Assert(val == expected);
				expected++;
			}
		}
	});

	producer.join();
	consumer.join();

	Assert(q.IsLikelyEmpty());
}

Fact("MpmcQueue Multiple Producers And Consumers Deliver Every Item Exactly Once")
{
	const int kProducers = 4;
	const int kConsumers = 4;
	const int kPerProducer = 20000;
	const int kTotal = kProducers * kPerProducer;

	MpmcQueue<int> q(1024);
	std::vector<std::atomic<bool>> seen(kTotal);
	for (auto& b : seen)
		b = false;

	std::atomic<int> totalConsumed{ 0 };
	std::atomic<bool> corrupted{ false };

	std::thread producers[kProducers];
	for (int p = 0; p < kProducers; p++)
	{
		producers[p] = std::thread([&, p]() {
			for (int i = 0; i < kPerProducer; i++)
			{
				int val = p * kPerProducer + i;
				while (!q.TryWrite(val)) {}
			}
		});
	}

	std::thread consumers[kConsumers];
	for (int c = 0; c < kConsumers; c++)
	{
		consumers[c] = std::thread([&]() {
			int val;
			while (totalConsumed < kTotal)
			{
				if (q.Read(val))
				{
					if (val < 0 || val >= kTotal || seen[val].exchange(true))
						corrupted = true;
					totalConsumed++;
				}
			}
		});
	}

	for (auto& t : producers)
		t.join();
	for (auto& t : consumers)
		t.join();

	Assert(!corrupted);
	Assert(totalConsumed == kTotal);
	for (int i = 0; i < kTotal; i++)
		Assert(seen[i]);

	Assert(q.IsLikelyEmpty());
}
