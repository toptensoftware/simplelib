#include <thread>
#include <atomic>
#include <vector>
#include "../UnitTesting.h"
#include "../Threading.h"
using namespace SimpleLib;

struct StackNode
{
	StackNode* next = nullptr;
	int value = 0;
};

Fact("MpmcStack Push Pop Single Item")
{
	MpmcStack<StackNode> s;
	StackNode n;
	n.value = 42;

	Assert(s.Push(&n) == true);	// transitioned from empty to non-empty

	bool nowEmpty = false;
	StackNode* popped = s.Pop(nowEmpty);
	Assert(popped == &n);
	Assert(popped->value == 42);
	Assert(nowEmpty == true);
}

Fact("MpmcStack Pop Empty Returns Null")
{
	MpmcStack<StackNode> s;
	Assert(s.Pop() == nullptr);

	bool nowEmpty = false;
	Assert(s.Pop(nowEmpty) == nullptr);
}

Fact("MpmcStack Push Does Not Corrupt Empty Stack")
{
	// Regression test: Push() into an empty stack used to leave the pushed
	// node's `next` pointing at itself (a self-referential cycle), because
	// the CAS success check was wrong. If that regresses, the second Pop()
	// below either returns the same node again or spins forever.
	MpmcStack<StackNode> s;
	StackNode n;

	s.Push(&n);

	bool nowEmpty = false;
	StackNode* popped = s.Pop(nowEmpty);
	Assert(popped == &n);
	Assert(nowEmpty == true);
	Assert(popped->next == nullptr);

	Assert(s.Pop() == nullptr);
}

Fact("MpmcStack Pop Order Is Lifo")
{
	MpmcStack<StackNode> s;
	StackNode n1, n2, n3;
	n1.value = 1;
	n2.value = 2;
	n3.value = 3;

	s.Push(&n1);
	s.Push(&n2);
	s.Push(&n3);

	Assert(s.Pop()->value == 3);
	Assert(s.Pop()->value == 2);
	Assert(s.Pop()->value == 1);
	Assert(s.Pop() == nullptr);
}

Fact("MpmcStack Push Reports Empty To Non Empty Transition")
{
	MpmcStack<StackNode> s;
	StackNode n1, n2;

	Assert(s.Push(&n1) == true);	// first item: was empty
	Assert(s.Push(&n2) == false);	// already had an item: wasn't empty
}

Fact("MpmcStack Push Chain")
{
	MpmcStack<StackNode> s;
	StackNode n1, n2, n3;
	n1.value = 1;
	n2.value = 2;
	n3.value = 3;
	n1.next = &n2;
	n2.next = &n3;
	n3.next = nullptr;

	Assert(s.Push(&n1, &n3) == true);
	Assert(s.GetLikelyCount() == 3);

	// Chain is pushed as a unit, preserving its internal order on top of the stack
	Assert(s.Pop()->value == 1);
	Assert(s.Pop()->value == 2);
	Assert(s.Pop()->value == 3);
	Assert(s.Pop() == nullptr);
}

Fact("MpmcStack Push Chain On Top Of Existing Items")
{
	MpmcStack<StackNode> s;
	StackNode base;
	base.value = 100;
	s.Push(&base);

	StackNode n1, n2;
	n1.value = 1;
	n2.value = 2;
	n1.next = &n2;
	n2.next = nullptr;

	Assert(s.Push(&n1, &n2) == false);	// stack wasn't empty before this push

	Assert(s.Pop()->value == 1);
	Assert(s.Pop()->value == 2);
	Assert(s.Pop()->value == 100);
}

Fact("MpmcStack GetLikelyCount Tracks Push And Pop")
{
	MpmcStack<StackNode> s;
	StackNode n1, n2, n3;

	Assert(s.GetLikelyCount() == 0);
	s.Push(&n1);
	Assert(s.GetLikelyCount() == 1);
	s.Push(&n2);
	s.Push(&n3);
	Assert(s.GetLikelyCount() == 3);

	s.Pop();
	Assert(s.GetLikelyCount() == 2);
	s.Pop();
	s.Pop();
	Assert(s.GetLikelyCount() == 0);
}

Fact("MpmcStack PopAll Returns Entire Chain And Empties Stack")
{
	MpmcStack<StackNode> s;
	StackNode n1, n2, n3;
	n1.value = 1;
	n2.value = 2;
	n3.value = 3;

	s.Push(&n1);
	s.Push(&n2);
	s.Push(&n3);

	StackNode* head = s.PopAll();
	Assert(head->value == 3);
	Assert(head->next->value == 2);
	Assert(head->next->next->value == 1);
	Assert(head->next->next->next == nullptr);

	Assert(s.GetLikelyCount() == 0);
	Assert(s.Pop() == nullptr);
}

Fact("MpmcStack PopAll On Empty Stack Returns Null")
{
	MpmcStack<StackNode> s;
	Assert(s.PopAll() == nullptr);
}

Fact("MpmcStack Reset Clears Items And Count")
{
	MpmcStack<StackNode> s;
	StackNode n1, n2;
	s.Push(&n1);
	s.Push(&n2);

	s.Reset();
	Assert(s.GetLikelyCount() == 0);
	Assert(s.Pop() == nullptr);
}

Fact("MpmcStack Multiple Producers And Consumers Deliver Every Item Exactly Once")
{
	const int kProducers = 4;
	const int kConsumers = 4;
	const int kPerProducer = 20000;
	const int kTotal = kProducers * kPerProducer;

	MpmcStack<StackNode> s;
	std::vector<StackNode> nodes(kTotal);
	std::vector<std::atomic<bool>> seen(kTotal);
	for (int i = 0; i < kTotal; i++)
	{
		nodes[i].value = i;
		seen[i] = false;
	}

	std::atomic<int> totalConsumed{ 0 };
	std::atomic<bool> corrupted{ false };

	std::thread producers[kProducers];
	for (int p = 0; p < kProducers; p++)
	{
		producers[p] = std::thread([&, p]() {
			for (int i = 0; i < kPerProducer; i++)
			{
				int idx = p * kPerProducer + i;
				s.Push(&nodes[idx]);
			}
		});
	}

	std::thread consumers[kConsumers];
	for (int c = 0; c < kConsumers; c++)
	{
		consumers[c] = std::thread([&]() {
			while (totalConsumed < kTotal)
			{
				StackNode* item = s.Pop();
				if (item)
				{
					int val = item->value;
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

	Assert(s.GetLikelyCount() == 0);
	Assert(s.Pop() == nullptr);
}

Fact("MpmcStack Concurrent Pushes Do Not Corrupt The Chain")
{
	// Regression test: a racing Push() used to be able to conclude its own
	// CAS had failed (and so retry) even though it had actually already
	// succeeded and published the node - because the success check
	// re-read the node's `next` field *after* the CAS, by which point a
	// concurrent Pop() could have already claimed the node and mutated
	// that same field out from under it. The retry then republished a
	// node a consumer had already been handed, corrupting the chain (one
	// node duplicated, another silently dropped). This test hammers the
	// push side alone (no concurrent pops), which is when the race is
	// most likely to bite.
	const int kProducers = 4;
	const int kPerProducer = 20000;
	const int kTotal = kProducers * kPerProducer;

	MpmcStack<StackNode> s;
	std::vector<StackNode> nodes(kTotal);
	for (int i = 0; i < kTotal; i++)
		nodes[i].value = i;

	std::thread producers[kProducers];
	for (int p = 0; p < kProducers; p++)
	{
		producers[p] = std::thread([&, p]() {
			for (int i = 0; i < kPerProducer; i++)
			{
				int idx = p * kPerProducer + i;
				s.Push(&nodes[idx]);
			}
		});
	}

	for (auto& t : producers)
		t.join();

	Assert(s.GetLikelyCount() == kTotal);

	std::vector<bool> seen(kTotal, false);
	int count = 0;
	StackNode* chain = s.PopAll();
	while (chain)
	{
		Assert(chain->value >= 0 && chain->value < kTotal);
		Assert(!seen[chain->value]);	// not already visited (no duplicate/cycle)
		seen[chain->value] = true;
		count++;
		chain = chain->next;
	}

	Assert(count == kTotal);	// none silently dropped
	Assert(s.GetLikelyCount() == 0);
}
