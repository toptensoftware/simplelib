#include <thread>
#include <atomic>
#include "../UnitTesting.h"
#include "../Threading.h"
using namespace SimpleLib;

namespace
{
	// An object that tracks how many instances are live and carries a magic
	// word, so a reader can spot a use-after-free / torn hand-off. Has the
	// `next` link AtomicPtrTransfer's return stack requires.
	struct Msg
	{
		static constexpr int kMagic = 0x5A5AA5A5;

		explicit Msg(int id) : Id(id) { s_live.fetch_add(1); }
		~Msg() { Magic = 0; s_live.fetch_sub(1); }

		Msg(const Msg&) = delete;
		Msg& operator=(const Msg&) = delete;

		Msg* next = nullptr;
		int Magic = kMagic;
		int Id;

		inline static std::atomic<int> s_live{0};
	};

	// A custom disposal policy: counts calls, then deletes.
	struct CountingRelease
	{
		static void Release(Msg* p)
		{
			if (p)
				s_releases.fetch_add(1);
			delete p;
		}

		inline static std::atomic<int> s_releases{0};
	};
}

Fact("AtomicPtrTransfer Receive And Reclaim Are Null When Idle")
{
	AtomicPtrTransfer<Msg> x;
	Assert(x.Receive() == nullptr);
	Assert(x.Reclaim() == nullptr);
}

Fact("AtomicPtrTransfer Receive Returns The Sent Object Once")
{
	Msg::s_live = 0;
	{
		AtomicPtrTransfer<Msg> x;

		Msg* a = new Msg(1);
		Assert(x.Send(a) == nullptr);		// nothing to hand back on the first send

		Assert(x.Receive() == a);
		Assert(x.Receive() == nullptr);		// already taken

		delete a;
	}
	Assert(Msg::s_live == 0);
}

Fact("AtomicPtrTransfer Receiver Holds Old And New Together Then Returns Old")
{
	Msg::s_live = 0;
	{
		AtomicPtrTransfer<Msg> x;

		Msg* current = new Msg(0);			// receiver's starting object

		for (int i = 1; i <= 20; i++)
		{
			Msg* sent = new Msg(i);
			Assert(x.Send(sent) == nullptr);

			Msg* incoming = x.Receive();
			Assert(incoming == sent);

			// Both objects are in the receiver's hands at the same time.
			Assert(current->Magic == Msg::kMagic);
			Assert(incoming->Magic == Msg::kMagic);
			Assert(incoming->Id == current->Id + 1);

			x.Return(current);
			current = incoming;

			Msg* back = x.Reclaim();
			Assert(back != nullptr && back->Id == i - 1);
			Assert(back->next == nullptr);	// link cleared on the way out
			delete back;
		}

		delete current;
	}
	Assert(Msg::s_live == 0);
}

Fact("AtomicPtrTransfer Return Stacks Many Objects Without Losing Any")
{
	// The bug the single-slot return channel had: several Return()s with no
	// Reclaim() in between must not overwrite/lose earlier ones.
	Msg::s_live = 0;
	{
		AtomicPtrTransfer<Msg> x;

		const int kN = 50;
		for (int i = 0; i < kN; i++)
			x.Return(new Msg(i));

		bool seen[kN] = {};
		int count = 0;
		while (Msg* r = x.Reclaim())
		{
			Assert(r->Magic == Msg::kMagic);
			Assert(r->Id >= 0 && r->Id < kN && !seen[r->Id]);
			seen[r->Id] = true;
			count++;
			delete r;
		}

		Assert(count == kN);
	}
	Assert(Msg::s_live == 0);
}

Fact("AtomicPtrTransfer Reclaim Picks Up Objects Returned After An Earlier Drain")
{
	Msg::s_live = 0;
	{
		AtomicPtrTransfer<Msg> x;

		x.Return(new Msg(1));
		Msg* a = x.Reclaim();
		Assert(a && a->Id == 1);
		delete a;
		Assert(x.Reclaim() == nullptr);

		x.Return(new Msg(2));
		x.Return(new Msg(3));
		Msg* b = x.Reclaim();
		Msg* c = x.Reclaim();
		Assert(b && c);
		Assert((b->Id == 2 && c->Id == 3) || (b->Id == 3 && c->Id == 2));
		Assert(x.Reclaim() == nullptr);
		delete b;
		delete c;
	}
	Assert(Msg::s_live == 0);
}

Fact("AtomicPtrTransfer Sender Outpacing Receiver Gets The Stale Object Back")
{
	Msg::s_live = 0;
	{
		AtomicPtrTransfer<Msg> x;

		Msg* a = new Msg(1);
		Msg* b = new Msg(2);

		Assert(x.Send(a) == nullptr);
		Assert(x.Send(b) == a);				// receiver never took 'a' - handed straight back
		Assert(x.Receive() == b);			// receiver only ever sees the latest

		delete a;
		delete b;
	}
	Assert(Msg::s_live == 0);
}

Fact("AtomicPtrTransfer Destructor Releases Both Channels")
{
	Msg::s_live = 0;
	CountingRelease::s_releases = 0;
	{
		AtomicPtrTransfer<Msg, CountingRelease> x;

		x.Send(new Msg(1));					// parked in the forward slot, never received

		x.Return(new Msg(2));				// on the return stack
		x.Return(new Msg(3));

		x.Return(new Msg(4));
		Msg* r = x.Reclaim();				// pulls the stack into m_reclaimList, hands out one
		Assert(r != nullptr);
		delete r;							// caller disposes it itself, not via TRelease

		Assert(Msg::s_live == 3);			// 1 in forward slot, 2 still in m_reclaimList
	}
	// destructor disposes the forward slot (1) and the leftover reclaim list (2)
	Assert(CountingRelease::s_releases == 3);
	Assert(Msg::s_live == 0);
}

Fact("AtomicPtrTransfer Threaded Hand-Off Loses No Objects And Never Tears")
{
	Msg::s_live = 0;

	const int kCount = 300000;
	std::atomic<bool> stop{false};
	std::atomic<bool> corruption{false};
	std::atomic<bool> outOfOrder{false};
	Msg* held = nullptr;

	{
		AtomicPtrTransfer<Msg> x;

		std::thread sender([&]()
		{
			for (int i = 1; i <= kCount; i++)
			{
				delete x.Send(new Msg(i));	// allocate + free only on this thread
				while (Msg* r = x.Reclaim())
					delete r;
			}
			stop = true;
		});

		std::thread receiver([&]()
		{
			Msg* current = nullptr;
			int lastId = 0;

			auto poll = [&]()
			{
				Msg* got = x.Receive();
				if (!got)
					return;
				if (got->Magic != Msg::kMagic)
					corruption = true;
				if (got->Id <= lastId)		// ids strictly increase (some are skipped)
					outOfOrder = true;
				lastId = got->Id;
				if (current)
					x.Return(current);		// old object handed back; 'got' kept
				current = got;
			};

			while (!stop)
				poll();

			poll();							// one last look after the sender stopped
			held = current;
		});

		sender.join();
		receiver.join();

		// Single-threaded again: account for everything still in flight.
		delete held;
		delete x.Receive();					// last unreceived send, if any
		while (Msg* r = x.Reclaim())			// return stack + leftover reclaim list
			delete r;
	}

	Assert(!corruption);
	Assert(!outOfOrder);
	Assert(Msg::s_live == 0);
}
