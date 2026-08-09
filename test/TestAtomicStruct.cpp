#include <thread>
#include <atomic>
#include "../UnitTesting.h"
#include "../Threading.h"
using namespace SimpleLib;

struct Point
{
	int x;
	int y;
};

Fact("AtomicStruct Get Set")
{
	AtomicStruct<Point> as(Point{ 1, 2 });

	Point p;
	as.Get(p);
	Assert(p.x == 1 && p.y == 2);

	as.Set(Point{ 3, 4 });
	as.Get(p);
	Assert(p.x == 3 && p.y == 4);
}

Fact("AtomicStruct Repeated Sets Always Reflect Latest Value")
{
	AtomicStruct<Point> as(Point{ 0, 0 });

	for (int i = 1; i <= 20; i++)
	{
		as.Set(Point{ i, i * 2 });
		Point p;
		as.Get(p);
		Assert(p.x == i);
		Assert(p.y == i * 2);
	}
}

Fact("AtomicStruct Reader Never Sees A Torn Write")
{
	// The struct's invariant is x == -y. A reader on another thread must
	// never observe a value where that doesn't hold, even while a writer
	// is continuously replacing the value concurrently.
	AtomicStruct<Point> as(Point{ 0, 0 });
	std::atomic<bool> stop(false);
	std::atomic<bool> tornWriteSeen(false);
	std::atomic<int> readCount(0);

	std::thread writer([&]() {
		for (int i = 0; i < 200000; i++)
			as.Set(Point{ i, -i });
		stop = true;
	});

	std::thread reader([&]() {
		Point p;
		while (!stop)
		{
			as.Get(p);
			readCount++;
			if (p.x != -p.y)
				tornWriteSeen = true;
		}
	});

	writer.join();
	reader.join();

	Assert(!tornWriteSeen);
	Assert(readCount > 0);
}
