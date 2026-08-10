#include <thread>
#include <atomic>
#include <vector>
#include <string.h>
#include "../UnitTesting.h"
#include "../Threading.h"
using namespace SimpleLib;

Fact("HighWaterHeapSet Alloc Returns Usable Memory")
{
	HighWaterHeapSet pool(2, 256);
	void* p = pool.Alloc(32);
	Assert(p != nullptr);

	memset(p, 0x5A, 32);
	Assert(((unsigned char*)p)[0] == 0x5A);
	Assert(((unsigned char*)p)[31] == 0x5A);

	pool.Free(p);
}

Fact("HighWaterHeapSet Reuses Same Bucket For Repeated Allocations")
{
	HighWaterHeapSet pool(2, 256);
	void* p1 = pool.Alloc(16);
	void* p2 = pool.Alloc(16);
	Assert(p1 && p2 && p1 != p2);

	// Both allocations should come from the same (active) bucket rather
	// than each pulling a fresh one
	Assert(HighWaterHeap::FromAllocation(p1) == HighWaterHeap::FromAllocation(p2));

	pool.Free(p1);
	pool.Free(p2);
}

Fact("HighWaterHeapSet Overflows To Next Bucket When Full")
{
	// bucketSize=128: Alloc(16) needs 16 + header(8) rounded up to 16 = 32
	// bytes each, so exactly 4 fit per bucket.
	HighWaterHeapSet pool(2, 128);

	void* ptrs[5];
	for (int i = 0; i < 4; i++)
	{
		ptrs[i] = pool.Alloc(16);
		Assert(ptrs[i] != nullptr);
	}

	// First 4 should all share the same (first) bucket
	for (int i = 1; i < 4; i++)
		Assert(HighWaterHeap::FromAllocation(ptrs[i]) == HighWaterHeap::FromAllocation(ptrs[0]));

	// 5th doesn't fit in the first bucket - must come from the second
	ptrs[4] = pool.Alloc(16);
	Assert(ptrs[4] != nullptr);
	Assert(HighWaterHeap::FromAllocation(ptrs[4]) != HighWaterHeap::FromAllocation(ptrs[0]));

	for (int i = 0; i < 5; i++)
		pool.Free(ptrs[i]);
}

Fact("HighWaterHeapSet Alloc Fails When Pool Fully Exhausted")
{
	// 2 buckets x 128 bytes, 4 allocations of 32 bytes fit per bucket -
	// exactly 8 allocations exhaust the whole pool (no bucket left to grow
	// into - this pool never allocates buckets beyond its initial set).
	HighWaterHeapSet pool(2, 128);

	void* ptrs[8];
	for (int i = 0; i < 8; i++)
	{
		ptrs[i] = pool.Alloc(16);
		Assert(ptrs[i] != nullptr);
	}

	Assert(pool.Alloc(16) == nullptr);

	for (int i = 0; i < 8; i++)
		pool.Free(ptrs[i]);
}

Fact("HighWaterHeapSet Drained Bucket Returns To Reserve And Stays Usable")
{
	// Regression test: when Alloc() found an active bucket already fully
	// drained (GetLikelyUsed() == 0), it used to push that bucket onto the
	// reserve stack *and* keep using the same object - re-linking its
	// intrusive `next` field into the active stack's chain right after
	// linking it into the reserve stack's. Since a Bucket can only belong to
	// one lock-free stack at a time, this cross-linked the two stacks
	// (dropped/aliased buckets, potential cycles). This test fills the pool
	// completely, drains one bucket back to empty while it's still sitting
	// in the active chain (not the one on top), and then allocates again -
	// which is exactly the sequence that used to hit the buggy path.
	HighWaterHeapSet pool(2, 128);

	void* ptrs[8];
	for (int i = 0; i < 8; i++)
	{
		ptrs[i] = pool.Alloc(16);
		Assert(ptrs[i] != nullptr);
	}
	Assert(pool.Alloc(16) == nullptr);	// fully exhausted

	// Free everything from the first bucket only, draining it to empty
	// while the second (later-allocated, and therefore higher in the
	// active stack) bucket is still full
	HighWaterHeap* bucket0 = HighWaterHeap::FromAllocation(ptrs[0]);
	for (int i = 0; i < 4; i++)
	{
		Assert(HighWaterHeap::FromAllocation(ptrs[i]) == bucket0);
		pool.Free(ptrs[i]);
	}

	// The pool must be able to satisfy new allocations again, and they
	// must be valid, non-overlapping memory
	void* fresh[4];
	for (int i = 0; i < 4; i++)
	{
		fresh[i] = pool.Alloc(16);
		Assert(fresh[i] != nullptr);
		memset(fresh[i], 0xA0 + i, 16);
	}
	for (int i = 0; i < 4; i++)
	{
		unsigned char* bytes = (unsigned char*)fresh[i];
		for (int b = 0; b < 16; b++)
			Assert(bytes[b] == (unsigned char)(0xA0 + i));
	}

	for (int i = 0; i < 4; i++)
		pool.Free(fresh[i]);
	for (int i = 4; i < 8; i++)
		pool.Free(ptrs[i]);
}

Fact("HighWaterHeapSet Free Via FromAllocation Routing Works Across Buckets")
{
	HighWaterHeapSet pool(2, 128);

	void* ptrs[5];
	for (int i = 0; i < 5; i++)
		ptrs[i] = pool.Alloc(16);
	for (int i = 0; i < 5; i++)
		Assert(ptrs[i] != nullptr);

	// ptrs[4] came from the second bucket (see the overflow test) - freeing
	// through the pool must route to the correct underlying heap regardless
	for (int i = 0; i < 5; i++)
		pool.Free(ptrs[i]);
}

Fact("HighWaterHeapSet Destructs Cleanly With Outstanding Buckets")
{
	// Regression test: the destructor used to do nothing, leaking every
	// Bucket ever created. This just needs to not crash - buckets sitting
	// untouched in reserve, and buckets holding live allocations, must both
	// be cleaned up.
	HighWaterHeapSet* pool = new HighWaterHeapSet(3, 128);
	void* p1 = pool->Alloc(16);
	void* p2 = pool->Alloc(16);
	Assert(p1 && p2);
	pool->Free(p1);

	delete pool;
	Assert(true);	// must not crash
}

Fact("HighWaterHeapSet Concurrent Alloc Free Does Not Corrupt Data")
{
	const int kThreads = 8;
	const int kIterations = 50000;
	const size_t kAllocSize = 16;

	// Small pool relative to the threads (each thread holds at most one
	// allocation at a time, so worst case concurrent demand is
	// kThreads * 32 bytes = 128 bytes, comfortably under a single bucket)
	// but with small buckets so they fill and drain constantly - this is
	// exactly the churn that exercises the active/reserve hand-off under
	// real concurrency.
	HighWaterHeapSet pool(2, 64);

	std::atomic<bool> corrupted{ false };

	std::thread threads[kThreads];
	for (int t = 0; t < kThreads; t++)
	{
		threads[t] = std::thread([&, t]() {
			unsigned char pattern = (unsigned char)(t * 41 + 7);
			for (int i = 0; i < kIterations; i++)
			{
				void* p = nullptr;
				while (!p)
					p = pool.Alloc(kAllocSize);

				memset(p, pattern, kAllocSize);
				std::this_thread::yield();

				unsigned char* bytes = (unsigned char*)p;
				for (size_t b = 0; b < kAllocSize; b++)
				{
					if (bytes[b] != pattern)
						corrupted = true;
				}

				pool.Free(p);
			}
		});
	}

	for (auto& th : threads)
		th.join();

	Assert(!corrupted);
}
