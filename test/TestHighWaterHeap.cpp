#include <thread>
#include <atomic>
#include <vector>
#include <string.h>
#include "../UnitTesting.h"
#include "../Threading.h"
using namespace SimpleLib;

Fact("HighWaterHeap Alloc Returns Usable Memory")
{
	HighWaterHeap heap(256);
	void* p = heap.Alloc(32);
	Assert(p != nullptr);

	memset(p, 0x42, 32);
	Assert(((unsigned char*)p)[0] == 0x42);
	Assert(((unsigned char*)p)[31] == 0x42);

	heap.Free(p);
}

Fact("HighWaterHeap GetCapacity Reflects Constructor Argument")
{
	HighWaterHeap heap(512);
	Assert(heap.GetCapacity() == 512);
}

Fact("HighWaterHeap GetLikelyUsed Increases After Alloc")
{
	HighWaterHeap heap(256);
	Assert(heap.GetLikelyUsed() == 0);

	void* p1 = heap.Alloc(32);
	Assert(p1 != nullptr);
	size_t usedAfter1 = heap.GetLikelyUsed();
	Assert(usedAfter1 > 0);

	void* p2 = heap.Alloc(32);
	Assert(p2 != nullptr);
	Assert(heap.GetLikelyUsed() > usedAfter1);

	heap.Free(p1);
	heap.Free(p2);
}

Fact("HighWaterHeap Alloc Fails When Full")
{
	HighWaterHeap heap(64);
	void* p1 = heap.Alloc(32);
	Assert(p1 != nullptr);

	void* p2 = heap.Alloc(64);	// definitely won't fit
	Assert(p2 == nullptr);

	// A failed alloc must not have disturbed the heap's state
	Assert(heap.GetLikelyUsed() > 0);
	heap.Free(p1);
	Assert(heap.GetLikelyUsed() == 0);
}

Fact("HighWaterHeap Sequential Allocations Do Not Overlap")
{
	// Regression test: Alloc() used to place the allocation's header (and
	// therefore the returned user pointer) at the *new* high-water mark
	// rather than the *old* one, so every allocation's memory overlapped
	// whatever the following Alloc() call handed out.
	HighWaterHeap heap(1024);
	void* p1 = heap.Alloc(32);
	void* p2 = heap.Alloc(32);
	void* p3 = heap.Alloc(32);
	Assert(p1 && p2 && p3);
	Assert(p1 != p2 && p2 != p3 && p1 != p3);

	memset(p1, 0xAA, 32);
	memset(p2, 0xBB, 32);
	memset(p3, 0xCC, 32);

	for (int i = 0; i < 32; i++)
	{
		Assert(((unsigned char*)p1)[i] == 0xAA);
		Assert(((unsigned char*)p2)[i] == 0xBB);
		Assert(((unsigned char*)p3)[i] == 0xCC);
	}

	heap.Free(p1);
	heap.Free(p2);
	heap.Free(p3);
}

Fact("HighWaterHeap Alloc Exactly Filling Capacity Succeeds")
{
	// The allocation header is a single pointer (8 bytes on this 64-bit
	// build), and requests round up to a 16 byte boundary - requesting 8
	// bytes needs exactly 16 bytes total, so a 16 byte heap should be
	// able to satisfy exactly one such request with nothing left over.
	// This also regresses the old header-placement bug, which wrote the
	// header one-past-the-end of m_pmem whenever an allocation exactly
	// filled the remaining capacity.
	HighWaterHeap heap(16);
	void* p = heap.Alloc(8);
	Assert(p != nullptr);

	memset(p, 0x77, 8);
	for (int i = 0; i < 8; i++)
		Assert(((unsigned char*)p)[i] == 0x77);

	Assert(heap.GetLikelyUsed() == 16);
	Assert(heap.Alloc(1) == nullptr);	// no room left

	heap.Free(p);
	Assert(heap.GetLikelyUsed() == 0);
}

Fact("HighWaterHeap Freeing One Of Several Does Not Corrupt High Water Mark")
{
	// Regression test: Free() used to leave the new high-water mark
	// uninitialized (garbage) whenever it freed one of several
	// outstanding allocations (ie: the count didn't drop to zero).
	HighWaterHeap heap(1024);
	void* p1 = heap.Alloc(32);
	void* p2 = heap.Alloc(32);
	void* p3 = heap.Alloc(32);
	Assert(p1 && p2 && p3);

	size_t usedBefore = heap.GetLikelyUsed();

	memset(p2, 0xBB, 32);
	memset(p3, 0xCC, 32);

	heap.Free(p1);	// count 3 -> 2: high-water mark must not move

	Assert(heap.GetLikelyUsed() == usedBefore);

	for (int i = 0; i < 32; i++)
	{
		Assert(((unsigned char*)p2)[i] == 0xBB);
		Assert(((unsigned char*)p3)[i] == 0xCC);
	}

	// Heap must still be able to allocate more without stomping on the
	// still-live blocks
	void* p4 = heap.Alloc(32);
	Assert(p4 != nullptr);
	memset(p4, 0xDD, 32);

	for (int i = 0; i < 32; i++)
	{
		Assert(((unsigned char*)p2)[i] == 0xBB);
		Assert(((unsigned char*)p3)[i] == 0xCC);
		Assert(((unsigned char*)p4)[i] == 0xDD);
	}

	heap.Free(p2);
	heap.Free(p3);
	heap.Free(p4);
	Assert(heap.GetLikelyUsed() == 0);
}

Fact("HighWaterHeap Freeing All Resets High Water Mark To Zero")
{
	HighWaterHeap heap(256);
	void* p1 = heap.Alloc(32);
	void* p2 = heap.Alloc(32);
	Assert(heap.GetLikelyUsed() > 0);

	heap.Free(p1);
	Assert(heap.GetLikelyUsed() > 0);	// p2 still outstanding

	heap.Free(p2);
	Assert(heap.GetLikelyUsed() == 0);

	// Heap should be fully reusable from scratch afterwards
	void* p3 = heap.Alloc(32);
	Assert(p3 != nullptr);
	heap.Free(p3);
}

class NotifyingHeap : public HighWaterHeap
{
public:
	using HighWaterHeap::HighWaterHeap;
	int emptyCount = 0;
	int fullCount = 0;
	int triggerCount = 0;

	virtual void OnEmpty() override
	{
		emptyCount++;
	}

	virtual void OnFull() override
	{
		fullCount++;
	}

	virtual void OnTrigger() override
	{
		triggerCount++;
	}
};

Fact("HighWaterHeap OnEmpty Called Only When Last Allocation Freed")
{
	NotifyingHeap heap(256);
	void* p1 = heap.Alloc(32);
	void* p2 = heap.Alloc(32);
	Assert(heap.emptyCount == 0);

	heap.Free(p1);
	Assert(heap.emptyCount == 0);	// one allocation still outstanding

	heap.Free(p2);
	Assert(heap.emptyCount == 1);
	Assert(heap.GetLikelyUsed() == 0);
}

Fact("HighWaterHeap OnFull Called When Allocation Fails")
{
	NotifyingHeap heap(64);
	void* p1 = heap.Alloc(32);
	Assert(p1 != nullptr);
	Assert(heap.fullCount == 0);

	void* p2 = heap.Alloc(64);	// won't fit
	Assert(p2 == nullptr);
	Assert(heap.fullCount == 1);

	void* p3 = heap.Alloc(64);	// still won't fit
	Assert(p3 == nullptr);
	Assert(heap.fullCount == 2);	// called again on each failed attempt

	heap.Free(p1);
}

Fact("HighWaterHeap OnFull Not Called On Successful Alloc")
{
	NotifyingHeap heap(256);
	void* p = heap.Alloc(32);
	Assert(p != nullptr);
	Assert(heap.fullCount == 0);
	heap.Free(p);
}

Fact("HighWaterHeap Default Trigger Fires On First Allocation")
{
	// Default trigger level is 0, so the very first allocation of a
	// fill cycle (which necessarily takes high from 0 to something > 0)
	// should cross it
	NotifyingHeap heap(256);
	Assert(heap.triggerCount == 0);

	void* p1 = heap.Alloc(32);
	Assert(heap.triggerCount == 1);

	// Subsequent allocations in the same cycle don't cross it again
	void* p2 = heap.Alloc(32);
	Assert(heap.triggerCount == 1);

	heap.Free(p1);
	heap.Free(p2);
}

Fact("HighWaterHeap SetTrigger Fires Once On Crossing")
{
	NotifyingHeap heap(256);
	heap.SetTrigger(50);
	Assert(heap.triggerCount == 0);

	// Each Alloc(8) consumes exactly 16 bytes of high-water mark (an 8
	// byte header plus 8 requested bytes, already 16-byte aligned)
	void* p1 = heap.Alloc(8);	// high: 0 -> 16, still below 50
	Assert(heap.triggerCount == 0);

	void* p2 = heap.Alloc(8);	// high: 16 -> 32, still below 50
	Assert(heap.triggerCount == 0);

	void* p3 = heap.Alloc(8);	// high: 32 -> 48, still below 50
	Assert(heap.triggerCount == 0);

	void* p4 = heap.Alloc(8);	// high: 48 -> 64, crosses 50
	Assert(heap.GetLikelyUsed() == 64);
	Assert(heap.triggerCount == 1);

	void* p5 = heap.Alloc(8);	// already past the trigger, no re-fire
	Assert(heap.triggerCount == 1);

	heap.Free(p1);
	heap.Free(p2);
	heap.Free(p3);
	heap.Free(p4);
	heap.Free(p5);
}

Fact("HighWaterHeap SetTrigger Fires Again After A New Fill Cycle")
{
	NotifyingHeap heap(256);
	heap.SetTrigger(16);

	void* p1 = heap.Alloc(16);
	Assert(heap.triggerCount == 1);

	heap.Free(p1);	// count -> 0, high-water mark resets to 0
	Assert(heap.GetLikelyUsed() == 0);

	void* p2 = heap.Alloc(16);	// new cycle: crosses the trigger again
	Assert(heap.triggerCount == 2);

	heap.Free(p2);
}

Fact("HighWaterHeap FreeAll Resets Usage Immediately")
{
	HighWaterHeap heap(256);
	heap.Alloc(32);
	heap.Alloc(32);
	Assert(heap.GetLikelyUsed() > 0);

	heap.FreeAll();
	Assert(heap.GetLikelyUsed() == 0);

	void* p = heap.Alloc(32);
	Assert(p != nullptr);
	heap.Free(p);
}

Fact("HighWaterHeap Reset Same Capacity Clears Usage")
{
	HighWaterHeap heap(256);
	void* p = heap.Alloc(32);
	Assert(p != nullptr);
	Assert(heap.GetLikelyUsed() > 0);

	heap.Reset(256);
	Assert(heap.GetLikelyUsed() == 0);
	Assert(heap.GetCapacity() == 256);

	void* p2 = heap.Alloc(32);
	Assert(p2 != nullptr);
	heap.Free(p2);
}

Fact("HighWaterHeap Reset Different Capacity Resizes")
{
	HighWaterHeap heap(64);
	Assert(heap.GetCapacity() == 64);

	heap.Reset(256);
	Assert(heap.GetCapacity() == 256);
	Assert(heap.GetLikelyUsed() == 0);

	void* p = heap.Alloc(200);
	Assert(p != nullptr);
	heap.Free(p);
}

Fact("HighWaterHeap FromAllocation Returns Owning Heap")
{
	HighWaterHeap heapA(256);
	HighWaterHeap heapB(256);

	void* pa = heapA.Alloc(16);
	void* pb = heapB.Alloc(16);
	Assert(pa && pb);

	Assert(HighWaterHeap::FromAllocation(pa) == &heapA);
	Assert(HighWaterHeap::FromAllocation(pb) == &heapB);

	heapA.Free(pa);
	heapB.Free(pb);
}

Fact("HighWaterHeap FreeFromCorrectHeap Frees Via Owning Heap")
{
	HighWaterHeap heapA(256);
	HighWaterHeap heapB(256);

	void* pa = heapA.Alloc(16);
	void* pb = heapB.Alloc(16);
	Assert(pa && pb);
	size_t usedB = heapB.GetLikelyUsed();

	HighWaterHeap::FreeFromCorrectHeap(pa);

	Assert(heapA.GetLikelyUsed() == 0);	// heapA had only the one allocation
	Assert(heapB.GetLikelyUsed() == usedB);	// heapB untouched

	HighWaterHeap::FreeFromCorrectHeap(pb);
	Assert(heapB.GetLikelyUsed() == 0);
}

Fact("HighWaterHeap FreeFromCorrectHeap Null Is No Op")
{
	HighWaterHeap::FreeFromCorrectHeap(nullptr);	// must not crash
	Assert(true);
}

Fact("HighWaterHeap Concurrent Allocations From Multiple Threads Do Not Corrupt Data")
{
	const int kThreads = 4;
	const int kPerThread = 2000;
	const size_t kBlockSize = 32;

	// Comfortably larger than kThreads*kPerThread allocations of
	// kBlockSize (plus header/rounding overhead) so no thread ever sees
	// a spurious allocation failure under contention
	HighWaterHeap heap((uint32_t)(kThreads * kPerThread * 64));

	std::vector<void*> allocated(kThreads * kPerThread, nullptr);
	std::atomic<bool> allocFailed{ false };

	std::thread producers[kThreads];
	for (int t = 0; t < kThreads; t++)
	{
		producers[t] = std::thread([&, t]() {
			unsigned char pattern = (unsigned char)(t + 1);
			for (int i = 0; i < kPerThread; i++)
			{
				void* p = heap.Alloc(kBlockSize);
				if (!p)
				{
					allocFailed = true;
					continue;
				}
				memset(p, pattern, kBlockSize);
				allocated[t * kPerThread + i] = p;
			}
		});
	}
	for (auto& th : producers)
		th.join();

	Assert(!allocFailed);

	// Regression check for the header-placement bug: verify every block's
	// pattern is still intact and no thread's writes bled into another
	// thread's block
	for (int t = 0; t < kThreads; t++)
	{
		unsigned char pattern = (unsigned char)(t + 1);
		for (int i = 0; i < kPerThread; i++)
		{
			unsigned char* bytes = (unsigned char*)allocated[t * kPerThread + i];
			for (size_t b = 0; b < kBlockSize; b++)
				Assert(bytes[b] == pattern);
		}
	}

	// Regression check for the uninitialized-high bug: free everything
	// concurrently and confirm the accounting lands back on exactly zero
	std::thread freers[kThreads];
	for (int t = 0; t < kThreads; t++)
	{
		freers[t] = std::thread([&, t]() {
			for (int i = 0; i < kPerThread; i++)
				heap.Free(allocated[t * kPerThread + i]);
		});
	}
	for (auto& th : freers)
		th.join();

	Assert(heap.GetLikelyUsed() == 0);
}
