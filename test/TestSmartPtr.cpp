#include "../UnitTesting.h"
#include "../Core.h"
using namespace SimpleLib;

namespace
{
	struct Tracked
	{
		Tracked() { s_live++; }
		~Tracked() { s_live--; }
		inline static int s_live = 0;
	};
}

Fact("OwnedPtr assign raw pointer deletes previous")
{
	{
		OwnedPtr<Tracked> p(new Tracked());
		Tracked* q = new Tracked();
		Assert(Tracked::s_live == 2);
		Tracked* r = (p = q);
		Assert(r == q);
		Assert(p == q);
		Assert(Tracked::s_live == 1);
		p = (Tracked*)p;		// self-assign must not delete
		Assert(Tracked::s_live == 1);
	}
	Assert(Tracked::s_live == 0);
}

Fact("OwnedPtr move assign deletes previous")
{
	{
		OwnedPtr<Tracked> a(new Tracked());
		OwnedPtr<Tracked> b(new Tracked());
		Tracked* pb = b;
		OwnedPtr<Tracked>& ret = (a = static_cast<OwnedPtr<Tracked>&&>(b));
		Assert(&ret == &a);
		Assert(a == pb);
		Assert(!b);
		Assert(Tracked::s_live == 1);
		a = static_cast<OwnedPtr<Tracked>&&>(a);		// self move
		Assert(a == pb);
		Assert(Tracked::s_live == 1);
	}
	Assert(Tracked::s_live == 0);
}

Fact("SharedPtr move assign releases previous")
{
	{
		SharedPtr<Tracked> a(new Tracked());
		SharedPtr<Tracked> b(new Tracked());
		SharedPtr<Tracked> c(b);
		Assert(Tracked::s_live == 2);
		SharedPtr<Tracked>& ret = (a = static_cast<SharedPtr<Tracked>&&>(b));
		Assert(&ret == &a);
		Assert(!b);
		Assert(a == c);
		Assert(Tracked::s_live == 1);
	}
	Assert(Tracked::s_live == 0);
}
