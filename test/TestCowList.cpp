#include <thread>
#include <chrono>
#include <atomic>
#include "../UnitTesting.h"
#include "../Threading.h"
using namespace SimpleLib;

Fact("CowList Basic Add And GetAt")
{
	CowList<int> list;
	Assert(list.GetSize() == 0);

	list.Add(10);
	list.Add(20);
	list.Add(30);

	Assert(list.GetSize() == 3);
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
	Assert(list.GetSize() == 4);
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
	Assert(list.GetSize() == 2);
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
	Assert(list.GetSize() == 2);
	Assert(list.GetAt(0) == 1);
	Assert(list.GetAt(1) == 3);

	// Removing a value that isn't present must be a safe no-op
	list.Remove(999);
	Assert(list.GetSize() == 2);
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
	Assert(list.GetSize() == 0);

	list.Add(99);
	Assert(list.GetSize() == 1);
	Assert(list.GetAt(0) == 99);
}

Fact("CowList Snapshot Reflects Full Contents")
{
	CowList<int> list;
	list.Add(1);
	list.Add(2);
	list.Add(3);

	CowListSnapshot<int>& snap = list.GetSnapshot();
	Assert(snap.GetSize() == 3);
	Assert(snap.GetItem(0) == 1);
	Assert(snap.GetItem(1) == 2);
	Assert(snap.GetItem(2) == 3);
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
	Assert(snap.GetSize() == 1);
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
	Assert(snap.GetSize() == 3);
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
	Assert(snap.GetSize() == 2);
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
	Assert(snap.GetSize() == 2);
	Assert(snap.GetItem(0) == 1);
	Assert(snap.GetItem(1) == 2);
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
	Assert(snap.GetItem(0) == 2);
	Assert(snap.GetItem(1) == 3);
	Assert(snap.GetItem(2) == 1);
}

Fact("CowList Grows Beyond Initial Size")
{
	CowList<int> list(4, 4);
	for (int i = 0; i < 100; i++)
		list.Add(i);

	Assert(list.GetSize() == 100);
	for (int i = 0; i < 100; i++)
		Assert(list.GetAt(i) == i);

	CowListSnapshot<int>& snap = list.GetSnapshot();
	Assert(snap.GetSize() == 100);
	for (int i = 0; i < 100; i++)
		Assert(snap.GetItem(i) == i);
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
			if (list.GetSize() > 10)
				list.RemoveAt(0);
			if (list.GetSize() >= 2)
				list.Move(0, list.GetSize() - 1);
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
			for (int i = 0; i < snap.GetSize(); i++)
			{
				int v = snap.GetItem(i);
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
