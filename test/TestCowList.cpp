#include <thread>
#include <chrono>
#include <atomic>
#include "../UnitTesting.h"
#include "../Threading.h"
using namespace SimpleLib;

Fact("CowList Basic Add And GetAt")
{
	CowList<int> list;
	Assert(list.GetCount() == 0);

	list.Add(10);
	list.Add(20);
	list.Add(30);

	Assert(list.GetCount() == 3);
	Assert(list.GetAt(0) == 10);
	Assert(list[1] == 20);
	Assert(list.GetAt(2) == 30);
}

Fact("CowList InsertAt")
{
	CowList<int> list;
	list.Add(1);
	list.Add(2);
	list.Add(3);

	list.InsertAt(1, 100);
	Assert(list.GetCount() == 4);
	Assert(list.GetAt(0) == 1);
	Assert(list.GetAt(1) == 100);
	Assert(list.GetAt(2) == 2);
	Assert(list.GetAt(3) == 3);
}

Fact("CowList RemoveAt")
{
	CowList<int> list;
	list.Add(1);
	list.Add(2);
	list.Add(3);

	list.RemoveAt(1);
	Assert(list.GetCount() == 2);
	Assert(list.GetAt(0) == 1);
	Assert(list.GetAt(1) == 3);
}

Fact("CowList Remove By Value")
{
	CowList<int> list;
	list.Add(1);
	list.Add(2);
	list.Add(3);

	list.Remove(2);
	Assert(list.GetCount() == 2);
	Assert(list.GetAt(0) == 1);
	Assert(list.GetAt(1) == 3);

	// Removing a value that isn't present must be a safe no-op
	list.Remove(999);
	Assert(list.GetCount() == 2);
}

Fact("CowList Move")
{
	CowList<int> list;
	list.Add(0);
	list.Add(1);
	list.Add(2);
	list.Add(3);

	list.Move(3, 1);
	Assert(list.GetAt(0) == 0);
	Assert(list.GetAt(1) == 3);
	Assert(list.GetAt(2) == 1);
	Assert(list.GetAt(3) == 2);
}

Fact("CowList Find And IndexOf")
{
	CowList<int> list;
	list.Add(10);
	list.Add(20);
	list.Add(30);

	Assert(list.Find(20) == 1);
	Assert(list.IndexOf(20) == 1);
	Assert(list.Find(999) == -1);
	Assert(list.IndexOf(999) == -1);
}

Fact("CowList Reset")
{
	CowList<int> list;
	list.Add(1);
	list.Add(2);

	list.Reset();
	Assert(list.GetCount() == 0);

	list.Add(99);
	Assert(list.GetCount() == 1);
	Assert(list.GetAt(0) == 99);
}

Fact("CowList Snapshot Reflects Full Contents")
{
	CowList<int> list;
	list.Add(1);
	list.Add(2);
	list.Add(3);

	CowListSnapshot<int>& snap = list.GetSnapshot();
	Assert(snap.GetCount() == 3);
	Assert(snap.GetAt(0) == 1);
	Assert(snap.GetAt(1) == 2);
	Assert(snap.GetAt(2) == 3);
}

Fact("CowList First Snapshot Reports Everything As Inserted")
{
	CowList<int> list;
	list.Add(1);
	list.Add(2);
	list.Add(3);

	CowListSnapshot<int>& snap = list.GetSnapshot();
	Assert(snap.GetInsertedCount() == 3);
	Assert(snap.GetDeletedCount() == 0);
}

Fact("CowList Snapshot With No Changes Reports Nothing")
{
	CowList<int> list;
	list.Add(1);
	list.GetSnapshot();

	// No mutations since the last snapshot
	CowListSnapshot<int>& snap = list.GetSnapshot();
	Assert(snap.GetCount() == 1);
	Assert(snap.GetInsertedCount() == 0);
	Assert(snap.GetDeletedCount() == 0);
}

Fact("CowList Snapshot Reports Only Items Added Since Last Snapshot")
{
	CowList<int> list;
	list.Add(1);
	list.Add(2);
	list.GetSnapshot();	// baseline

	list.Add(3);
	CowListSnapshot<int>& snap = list.GetSnapshot();
	Assert(snap.GetCount() == 3);
	Assert(snap.GetInsertedCount() == 1);
	Assert(snap.GetInsertedItem(0) == 3);
	Assert(snap.GetDeletedCount() == 0);
}

Fact("CowList Snapshot Reports Only Items Removed Since Last Snapshot")
{
	CowList<int> list;
	list.Add(1);
	list.Add(2);
	list.Add(3);
	list.GetSnapshot();	// baseline

	list.RemoveAt(1);	// removes 2
	CowListSnapshot<int>& snap = list.GetSnapshot();
	Assert(snap.GetCount() == 2);
	Assert(snap.GetInsertedCount() == 0);
	Assert(snap.GetDeletedCount() == 1);
	Assert(snap.GetDeletedItem(0) == 2);
}

Fact("CowList Insert Then Remove Before Snapshot Cancels Out")
{
	CowList<int> list;
	list.Add(1);
	list.Add(2);
	list.GetSnapshot();	// baseline

	list.Add(3);
	list.RemoveAt(list.Find(3));

	CowListSnapshot<int>& snap = list.GetSnapshot();
	Assert(snap.GetCount() == 2);
	Assert(snap.GetAt(0) == 1);
	Assert(snap.GetAt(1) == 2);
	Assert(snap.GetInsertedCount() == 0);
	Assert(snap.GetDeletedCount() == 0);
}

Fact("CowList Move Does Not Appear In Inserted Or Deleted")
{
	CowList<int> list;
	list.Add(1);
	list.Add(2);
	list.Add(3);
	list.GetSnapshot();	// baseline

	list.Move(0, 2);
	CowListSnapshot<int>& snap = list.GetSnapshot();
	Assert(snap.GetInsertedCount() == 0);
	Assert(snap.GetDeletedCount() == 0);
	Assert(snap.GetAt(0) == 2);
	Assert(snap.GetAt(1) == 3);
	Assert(snap.GetAt(2) == 1);
}

Fact("CowList Grows Beyond Initial Capacity")
{
	CowList<int> list(4, 4);
	for (int i = 0; i < 100; i++)
		list.Add(i);

	Assert(list.GetCount() == 100);
	for (int i = 0; i < 100; i++)
		Assert(list.GetAt(i) == i);

	CowListSnapshot<int>& snap = list.GetSnapshot();
	Assert(snap.GetCount() == 100);
	for (int i = 0; i < 100; i++)
		Assert(snap.GetAt(i) == i);
}

// Deliberately has no operator== - only compiles against a CowList that
// doesn't require T to be comparable (TrackChanges = false).
struct NonComparablePoint
{
	int x;
	int y;
};

Fact("CowList TrackChanges False Compiles For Non Comparable T")
{
	CowList<NonComparablePoint, false> list;
	list.Add({ 1, 2 });
	list.Add({ 3, 4 });
	list.RemoveAt(0);

	Assert(list.GetCount() == 1);
	Assert(list.GetAt(0).x == 3);
	Assert(list.GetAt(0).y == 4);
}

Fact("CowList TrackChanges False Snapshot Reports Full Contents But No Inserted Deleted")
{
	CowList<NonComparablePoint, false> list;
	list.Add({ 1, 2 });
	list.GetSnapshot();	// baseline

	list.Add({ 3, 4 });
	list.RemoveAt(0);

	CowListSnapshot<NonComparablePoint, false>& snap = list.GetSnapshot();
	Assert(snap.GetCount() == 1);
	Assert(snap.GetAt(0).x == 3);
	Assert(snap.GetAt(0).y == 4);

	// Change tracking is disabled - always reports nothing, even though
	// items were in fact added/removed
	Assert(snap.GetInsertedCount() == 0);
	Assert(snap.GetDeletedCount() == 0);
}

Fact("CowList TrackChanges False StartUpdate EndUpdate Still Batches")
{
	CowList<NonComparablePoint, false> list;
	list.Add({ 1, 1 });
	list.GetSnapshot();	// baseline

	list.StartUpdate();
	list.Add({ 2, 2 });
	list.Add({ 3, 3 });
	list.EndUpdate();

	CowListSnapshot<NonComparablePoint, false>& snap = list.GetSnapshot();
	Assert(snap.GetCount() == 3);
	Assert(snap.GetInsertedCount() == 0);
}

Fact("CowList StartUpdate EndUpdate Batches Multiple Ops Into One Snapshot")
{
	CowList<int> list;
	list.Add(1);
	list.Add(2);
	list.GetSnapshot();	// baseline

	list.StartUpdate();
	list.Add(3);
	list.Add(4);
	list.RemoveAt(0);	// removes 1
	list.EndUpdate();

	CowListSnapshot<int>& snap = list.GetSnapshot();
	Assert(snap.GetCount() == 3);
	Assert(snap.GetAt(0) == 2);
	Assert(snap.GetAt(1) == 3);
	Assert(snap.GetAt(2) == 4);
	Assert(snap.GetInsertedCount() == 2);
	Assert(snap.GetDeletedCount() == 1);
	Assert(snap.GetDeletedItem(0) == 1);
}

Fact("CowList StartUpdate EndUpdate Nests")
{
	CowList<int> list;
	list.GetSnapshot();	// baseline

	list.StartUpdate();
	list.StartUpdate();
	list.Add(1);
	list.EndUpdate();
	Assert(list.GetCount() == 1);
	list.Add(2);
	list.EndUpdate();

	// Nothing should be visible until the outermost EndUpdate()
	CowListSnapshot<int>& snap = list.GetSnapshot();
	Assert(snap.GetCount() == 2);
	Assert(snap.GetInsertedCount() == 2);
}

Fact("CowList EndUpdate With No Changes Publishes Nothing New")
{
	CowList<int> list;
	list.Add(1);
	list.GetSnapshot();	// baseline

	list.StartUpdate();
	list.EndUpdate();

	CowListSnapshot<int>& snap = list.GetSnapshot();
	Assert(snap.GetCount() == 1);
	Assert(snap.GetInsertedCount() == 0);
	Assert(snap.GetDeletedCount() == 0);
}

Fact("CowList Reader Never Sees A Torn Batch")
{
	// Reader thread polls concurrently while the writer runs many
	// StartUpdate()/Add()xN/EndUpdate() batches. Each batch's net item
	// count is known ahead of time, so if the reader ever observes a
	// count that isn't one of the expected "settled" totals, it caught a
	// partially-applied batch.
	CowList<int> list;
	const int kBatches = 5000;
	const int kOpsPerBatch = 4;
	std::atomic<bool> done{ false };
	std::atomic<bool> tornBatchSeen{ false };

	std::thread writer([&]() {
		int next = 0;
		for (int b = 0; b < kBatches; b++)
		{
			list.StartUpdate();
			for (int i = 0; i < kOpsPerBatch; i++)
				list.Add(next++);
			list.EndUpdate();
		}
		done = true;
	});

	std::thread reader([&]() {
		while (!done)
		{
			CowListSnapshot<int>& snap = list.GetSnapshot();
			if (snap.GetCount() % kOpsPerBatch != 0)
				tornBatchSeen = true;
		}
	});

	writer.join();
	reader.join();

	Assert(!tornBatchSeen);
	Assert(list.GetCount() == kBatches * kOpsPerBatch);
}

Fact("CowList Writer Reader Threads Stay Consistent")
{
	// Simulates the intended usage pattern: one writer thread mutating,
	// one reader thread repeatedly taking snapshots, running concurrently.
	CowList<int> list;
	const int kOps = 20000;
	std::atomic<bool> done{ false };
	std::atomic<int> snapshotsTaken{ 0 };
	std::atomic<bool> inconsistent{ false };

	std::thread writer([&]() {
		int next = 0;
		for (int i = 0; i < kOps; i++)
		{
			list.Add(next++);
			if (list.GetCount() > 10)
				list.RemoveAt(0);
			if (list.GetCount() >= 2)
				list.Move(0, list.GetCount() - 1);
		}
		done = true;
	});

	std::thread reader([&]() {
		// Window is never more than 11 items, but values are ever-increasing
		// (never reused) up to kOps - track "seen" sparsely rather than
		// clearing a kOps-sized array every iteration.
		int touched[16];
		while (!done)
		{
			CowListSnapshot<int>& snap = list.GetSnapshot();

			// Every item in a snapshot must be within range and unique -
			// a torn read of the writer's buffer would likely violate this
			int touchedCount = 0;
			for (int i = 0; i < snap.GetCount(); i++)
			{
				int v = snap.GetAt(i);
				if (v < 0 || v >= kOps)
				{
					inconsistent = true;
					break;
				}
				for (int j = 0; j < touchedCount; j++)
				{
					if (touched[j] == v)
					{
						inconsistent = true;
						break;
					}
				}
				touched[touchedCount++] = v;
			}
			snapshotsTaken++;
		}
	});

	writer.join();
	reader.join();

	Assert(!inconsistent);
	Assert(snapshotsTaken > 0);
}
