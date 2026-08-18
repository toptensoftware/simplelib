#include <thread>
#include <atomic>
#include "../UnitTesting.h"
#include "../Threading.h"
using namespace SimpleLib;

// Tracks live instances so we can verify ThreadLocal::Free() actually runs
// (only wired up for SimpleLib::Thread, via Thread::ThreadProcStub calling
// ThreadLocalBase::FreeAll() after ThreadProc returns).
class TlsInstanceCounter
{
public:
	TlsInstanceCounter() { s_iInstances++; }
	~TlsInstanceCounter() { s_iInstances--; }

	inline static std::atomic<int> s_iInstances{ 0 };
};

Fact("ThreadLocal Get Creates Value Initialized Instance")
{
	ThreadLocal<int> tl;
	int* p = tl.Get();
	Assert(p != nullptr);
	Assert(*p == 0);
}

Fact("ThreadLocal Get Returns Same Instance On Same Thread")
{
	ThreadLocal<int> tl;
	int* p1 = tl.Get();
	*p1 = 42;
	int* p2 = tl.Get();
	Assert(p1 == p2);
	Assert(*p2 == 42);
}

Fact("ThreadLocal Different Threads Get Different Instances")
{
	ThreadLocal<int> tl;
	int* mainPtr = tl.Get();
	*mainPtr = 1;

	int* otherPtr = nullptr;
	int otherVal = -1;
	std::thread t([&]() {
		otherPtr = tl.Get();
		*otherPtr = 2;
		otherVal = *tl.Get();
	});
	t.join();

	Assert(otherPtr != nullptr);
	Assert(otherPtr != mainPtr);
	Assert(*mainPtr == 1);		// other thread's write didn't affect ours
	Assert(otherVal == 2);
}

class TlsThread : public Thread
{
public:
	ThreadLocal<TlsInstanceCounter>* tl;

	TlsThread(ThreadLocal<TlsInstanceCounter>* tl) : tl(tl) {}

	virtual void ThreadProc() override
	{
		tl->Get();	// force creation of this thread's instance
	}
};

Fact("ThreadLocal Free Called On SimpleLib Thread Exit")
{
	TlsInstanceCounter::s_iInstances = 0;
	ThreadLocal<TlsInstanceCounter> tl;

	TlsThread t(&tl);
	t.Start();
	t.Join();

	// ThreadProcStub must have called ThreadLocalBase::FreeAll() after
	// ThreadProc returned, destroying that thread's TlsInstanceCounter
	Assert(TlsInstanceCounter::s_iInstances == 0);
}

Fact("ThreadLocal Free Called Independently Per Thread")
{
	TlsInstanceCounter::s_iInstances = 0;
	ThreadLocal<TlsInstanceCounter> tl;

	const int kThreads = 4;
	TlsThread* threads[kThreads];
	for (int i = 0; i < kThreads; i++)
		threads[i] = new TlsThread(&tl);

	for (int i = 0; i < kThreads; i++)
		threads[i]->Start();
	for (int i = 0; i < kThreads; i++)
		threads[i]->Join();

	Assert(TlsInstanceCounter::s_iInstances == 0);

	for (int i = 0; i < kThreads; i++)
		delete threads[i];
}
