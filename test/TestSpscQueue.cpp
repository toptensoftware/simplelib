#include <thread>
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

Fact("SpscQueue Basic Write Read")
{
	SpscQueue<int> q(8);
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

Fact("SpscQueue MustWrite")
{
	SpscQueue<int> q(8);
	q.MustWrite(10);
	q.MustWrite(20);

	int val;
	Assert(q.Read(val) && val == 10);
	Assert(q.Read(val) && val == 20);
}

Fact("SpscQueue Fills Up To Capacity Minus One")
{
	// Note: one slot is always kept empty to distinguish full from empty,
	// so a queue constructed with size N can only ever hold N-1 items even
	// though GetCapacity() reports N.
	SpscQueue<int> q(4);
	Assert(q.GetCapacity() == 4);

	int count = 0;
	while (q.TryWrite(count))
		count++;

	Assert(count == q.GetCapacity() - 1);
	Assert(q.IsLikelyFull());
	Assert(!q.TryWrite(999));
}

Fact("SpscQueue TryWrite Succeeds Again After Read Frees A Slot")
{
	SpscQueue<int> q(4);
	while (q.TryWrite(0)) {}
	Assert(q.IsLikelyFull());

	int val;
	Assert(q.Read(val));
	Assert(!q.IsLikelyFull());
	Assert(q.TryWrite(999));
	Assert(q.IsLikelyFull());
}

Fact("SpscQueue Peek Does Not Remove")
{
	SpscQueue<int> q(8);
	q.TryWrite(42);

	int val = -1;
	Assert(q.Peek(val));
	Assert(val == 42);
	Assert(q.GetLikelyCount() == 1);		// still there

	Assert(q.Read(val));
	Assert(val == 42);
	Assert(q.IsLikelyEmpty());
}

Fact("SpscQueue Peek On Empty Fails")
{
	SpscQueue<int> q(8);
	int val;
	Assert(!q.Peek(val));
}

Fact("SpscQueue Peek With Offset")
{
	SpscQueue<int> q(8);
	q.TryWrite(10);
	q.TryWrite(20);
	q.TryWrite(30);

	int val;
	Assert(q.Peek(0, val) && val == 10);
	Assert(q.Peek(1, val) && val == 20);
	Assert(q.Peek(2, val) && val == 30);
	Assert(!q.Peek(-1, val));
	Assert(!q.Peek(3, val));		// out of range: only 3 items present
}

Fact("SpscQueue GetLikelyAt")
{
	SpscQueue<int> q(8);
	q.TryWrite(10);
	q.TryWrite(20);
	q.TryWrite(30);

	Assert(q.GetLikelyAt(0) == 10);
	Assert(q.GetLikelyAt(1) == 20);
	Assert(q.GetLikelyAt(2) == 30);
}

Fact("SpscQueue RemoveAll")
{
	SpscQueue<int> q(8);
	q.TryWrite(1);
	q.TryWrite(2);
	q.TryWrite(3);

	q.RemoveAll();
	Assert(q.IsLikelyEmpty());
	Assert(q.GetLikelyCount() == 0);

	// Still usable afterwards
	Assert(q.TryWrite(99));
	Assert(q.GetLikelyCount() == 1);
}

Fact("SpscQueue Reset Same Size")
{
	SpscQueue<int> q(8);
	q.TryWrite(1);
	q.TryWrite(2);

	q.Reset();
	Assert(q.IsLikelyEmpty());
	Assert(q.GetCapacity() == 8);

	Assert(q.TryWrite(5));
	Assert(q.GetLikelyCount() == 1);
}

Fact("SpscQueue Reset New Size")
{
	SpscQueue<int> q(8);
	q.TryWrite(1);

	q.Reset(16);
	Assert(q.IsLikelyEmpty());
	Assert(q.GetCapacity() == 16);
}

Fact("SpscQueue Reset Destroys Pending Elements")
{
	TrackedValue::s_iInstances = 0;
	{
		SpscQueue<TrackedValue> q(8);
		q.TryWrite(TrackedValue(1));
		q.TryWrite(TrackedValue(2));
		q.TryWrite(TrackedValue(3));
		Assert(TrackedValue::s_iInstances == 3);

		// Same capacity - must still destroy the abandoned elements
		q.Reset();
		Assert(TrackedValue::s_iInstances == 0);

		q.TryWrite(TrackedValue(4));
		Assert(TrackedValue::s_iInstances == 1);

		// New capacity - old buffer is freed, elements must be destroyed first
		q.Reset(16);
		Assert(TrackedValue::s_iInstances == 0);

		// Destructor must also destroy anything left in the queue
		q.TryWrite(TrackedValue(5));
		Assert(TrackedValue::s_iInstances == 1);
	}
	Assert(TrackedValue::s_iInstances == 0);
}

Fact("SpscQueue Wraparound Preserves FIFO Order")
{
	SpscQueue<int> q(4);	// small buffer forces frequent wraparound

	int nextWrite = 0;
	int nextRead = 0;
	int val;

	// Interleave writes and reads well past the buffer's physical size
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

Fact("SpscQueue Producer Consumer Threads Preserve Order And Count")
{
	SpscQueue<int> q(64);
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
