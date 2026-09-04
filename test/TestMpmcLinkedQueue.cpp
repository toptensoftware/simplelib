#include <thread>
#include <atomic>
#include <vector>
#include "../UnitTesting.h"
#include "../Threading.h"
using namespace SimpleLib;

namespace
{
	struct QueueNode
	{
		QueueNode* next = nullptr;
		int value = 0;
	};

	// Walk a FIFO chain returned by ReadAll(), collecting the values and
	// clearing each node's next pointer so the node can be written back in.
	std::vector<int> Drain(QueueNode* head)
	{
		std::vector<int> out;
		while (head)
		{
			QueueNode* next = head->next;
			head->next = nullptr;
			out.push_back(head->value);
			head = next;
		}
		return out;
	}
}

Fact("MpmcLinkedQueue ReadAll On Empty Queue Returns Null")
{
	MpmcLinkedQueue<QueueNode> q;
	Assert(q.ReadAll() == nullptr);
	Assert(q.GetLikelyCount() == 0);
}

Fact("MpmcLinkedQueue Write Then ReadAll Single Item")
{
	MpmcLinkedQueue<QueueNode> q;
	QueueNode n;
	n.value = 42;

	Assert(q.Write(&n) == true);		// transitioned from empty to non-empty
	Assert(q.GetLikelyCount() == 1);

	QueueNode* head = q.ReadAll();
	Assert(head == &n);
	Assert(head->value == 42);
	Assert(head->next == nullptr);		// chain is properly terminated

	Assert(q.ReadAll() == nullptr);		// drained
	Assert(q.GetLikelyCount() == 0);
}

Fact("MpmcLinkedQueue ReadAll Returns Items In FIFO Order")
{
	// The underlying stack is LIFO; the queue must reverse it so the first
	// item written is the head of the chain ReadAll() hands back.
	MpmcLinkedQueue<QueueNode> q;
	QueueNode n1, n2, n3;
	n1.value = 1;
	n2.value = 2;
	n3.value = 3;

	q.Write(&n1);
	q.Write(&n2);
	q.Write(&n3);

	QueueNode* head = q.ReadAll();
	Assert(head == &n1);
	Assert(head->next == &n2);
	Assert(head->next->next == &n3);
	Assert(head->next->next->next == nullptr);
}

Fact("MpmcLinkedQueue Write Reports Empty To Non Empty Transition")
{
	MpmcLinkedQueue<QueueNode> q;
	QueueNode n1, n2, n3;

	Assert(q.Write(&n1) == true);		// first item: was empty
	Assert(q.Write(&n2) == false);		// already had an item
	Assert(q.Write(&n3) == false);

	Drain(q.ReadAll());

	Assert(q.Write(&n1) == true);		// empty again after drain
}

Fact("MpmcLinkedQueue GetLikelyCount Tracks Writes And Resets After ReadAll")
{
	MpmcLinkedQueue<QueueNode> q;
	QueueNode n1, n2, n3;

	Assert(q.GetLikelyCount() == 0);
	q.Write(&n1);
	q.Write(&n2);
	q.Write(&n3);
	Assert(q.GetLikelyCount() == 3);

	q.ReadAll();
	Assert(q.GetLikelyCount() == 0);
}

Fact("MpmcLinkedQueue Reset Clears Pending Items")
{
	MpmcLinkedQueue<QueueNode> q;
	QueueNode n1, n2;
	q.Write(&n1);
	q.Write(&n2);

	q.Reset();
	Assert(q.GetLikelyCount() == 0);
	Assert(q.ReadAll() == nullptr);
}

Fact("MpmcLinkedQueue Interleaved Write And ReadAll Preserve Order Across Batches")
{
	MpmcLinkedQueue<QueueNode> q;
	std::vector<QueueNode> nodes(6);
	for (int i = 0; i < 6; i++)
		nodes[i].value = i;

	q.Write(&nodes[0]);
	q.Write(&nodes[1]);
	Assert((Drain(q.ReadAll()) == std::vector<int>{ 0, 1 }));

	q.Write(&nodes[2]);
	q.Write(&nodes[3]);
	q.Write(&nodes[4]);
	Assert((Drain(q.ReadAll()) == std::vector<int>{ 2, 3, 4 }));

	q.Write(&nodes[5]);
	Assert((Drain(q.ReadAll()) == std::vector<int>{ 5 }));

	Assert(q.ReadAll() == nullptr);
}

Fact("MpmcLinkedQueue Reused Nodes Cycle Through The Queue Cleanly")
{
	// A node popped out of a ReadAll() chain has next == nullptr, so it can
	// be written straight back in. Do that many times over a tiny node set.
	MpmcLinkedQueue<QueueNode> q;
	std::vector<QueueNode> nodes(3);
	for (int i = 0; i < 3; i++)
		nodes[i].value = i;

	for (int round = 0; round < 1000; round++)
	{
		for (auto& n : nodes)
			q.Write(&n);

		QueueNode* head = q.ReadAll();
		Assert((Drain(head) == std::vector<int>{ 0, 1, 2 }));
	}

	Assert(q.ReadAll() == nullptr);
	Assert(q.GetLikelyCount() == 0);
}

Fact("MpmcLinkedQueue Multiple Producers Draining Consumer Deliver Every Item Exactly Once")
{
	const int kProducers = 4;
	const int kPerProducer = 50000;
	const int kTotal = kProducers * kPerProducer;

	MpmcLinkedQueue<QueueNode> q;
	std::vector<QueueNode> nodes(kTotal);
	for (int i = 0; i < kTotal; i++)
		nodes[i].value = i;

	std::vector<std::atomic<bool>> seen(kTotal);
	for (auto& b : seen)
		b = false;

	std::atomic<int> totalConsumed{ 0 };
	std::atomic<bool> corrupted{ false };
	std::atomic<bool> stop{ false };

	std::thread producers[kProducers];
	for (int p = 0; p < kProducers; p++)
	{
		producers[p] = std::thread([&, p]() {
			for (int i = 0; i < kPerProducer; i++)
				q.Write(&nodes[p * kPerProducer + i]);
		});
	}

	// Single draining consumer: repeatedly grab whole batches.
	std::thread consumer([&]() {
		while (!stop.load() || totalConsumed.load() < kTotal)
		{
			QueueNode* head = q.ReadAll();

			// Per-producer FIFO: within one batch, values from the same
			// producer must appear in strictly increasing order.
			std::vector<int> lastFromProducer(kProducers, -1);

			while (head)
			{
				int val = head->value;
				if (val < 0 || val >= kTotal || seen[val].exchange(true))
					corrupted = true;

				int producer = val / kPerProducer;
				if (val <= lastFromProducer[producer])
					corrupted = true;			// out-of-order within this producer
				lastFromProducer[producer] = val;

				totalConsumed++;
				head = head->next;
			}
		}
	});

	for (auto& t : producers)
		t.join();
	stop = true;
	consumer.join();

	Assert(!corrupted);
	Assert(totalConsumed == kTotal);
	for (int i = 0; i < kTotal; i++)
		Assert(seen[i]);

	Assert(q.ReadAll() == nullptr);
	Assert(q.GetLikelyCount() == 0);
}

Fact("MpmcLinkedQueue Multiple Producers And Multiple Draining Consumers Lose No Items")
{
	const int kProducers = 4;
	const int kConsumers = 4;
	const int kPerProducer = 40000;
	const int kTotal = kProducers * kPerProducer;

	MpmcLinkedQueue<QueueNode> q;
	std::vector<QueueNode> nodes(kTotal);
	for (int i = 0; i < kTotal; i++)
		nodes[i].value = i;

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
				q.Write(&nodes[p * kPerProducer + i]);
		});
	}

	std::thread consumers[kConsumers];
	for (int c = 0; c < kConsumers; c++)
	{
		consumers[c] = std::thread([&]() {
			while (totalConsumed.load() < kTotal)
			{
				QueueNode* head = q.ReadAll();
				while (head)
				{
					int val = head->value;
					QueueNode* next = head->next;	// read before anyone reuses it
					if (val < 0 || val >= kTotal || seen[val].exchange(true))
						corrupted = true;
					totalConsumed++;
					head = next;
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

	Assert(q.ReadAll() == nullptr);
}
