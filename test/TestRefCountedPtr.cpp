#include "../UnitTesting.h"
#include "../Core.h"
using namespace SimpleLib;

// A minimal COM-like object: constructed with a reference count of 1,
// AddRef()/Release() manage its lifetime, and it deletes itself when the
// count drops to zero.
class RefCountedThing
{
public:
	RefCountedThing()
	{
		_iRef = 1;
		s_iInstances++;
	}

	int AddRef()
	{
		return ++_iRef;
	}

	int Release()
	{
		int iRef = --_iRef;
		if (iRef == 0)
			delete this;
		return iRef;
	}

	int GetRefCount() const { return _iRef; }

	inline static int s_iInstances = 0;

private:
	~RefCountedThing()
	{
		s_iInstances--;
	}

	int _iRef;
};

Fact("RefCountedPtr Adopts Reference Without AddRef")
{
	RefCountedThing::s_iInstances = 0;

	RefCountedThing* raw = new RefCountedThing();
	Assert(raw->GetRefCount() == 1);

	RefCountedPtr<RefCountedThing> p(raw);
	Assert(raw->GetRefCount() == 1);
	Assert(RefCountedThing::s_iInstances == 1);
}

Fact("RefCountedPtr Releases On Destruction")
{
	RefCountedThing::s_iInstances = 0;
	{
		RefCountedPtr<RefCountedThing> p(new RefCountedThing());
		Assert(RefCountedThing::s_iInstances == 1);
	}
	Assert(RefCountedThing::s_iInstances == 0);
}

Fact("RefCountedPtr Default Constructor")
{
	RefCountedPtr<RefCountedThing> p;
	Assert(!p);
	Assert((RefCountedThing*)p == nullptr);
}

Fact("RefCountedPtr Copy Constructor AddRefs")
{
	RefCountedThing::s_iInstances = 0;
	RefCountedThing* raw = new RefCountedThing();

	RefCountedPtr<RefCountedThing> a(raw);
	{
		RefCountedPtr<RefCountedThing> b(a);
		Assert(raw->GetRefCount() == 2);
		Assert(RefCountedThing::s_iInstances == 1);
	}

	// b's destruction released its reference, but a's is still alive
	Assert(raw->GetRefCount() == 1);
	Assert(RefCountedThing::s_iInstances == 1);
}

Fact("RefCountedPtr Copy Assignment AddRefs And Releases Old")
{
	RefCountedThing::s_iInstances = 0;
	RefCountedThing* rawA = new RefCountedThing();
	RefCountedThing* rawB = new RefCountedThing();
	Assert(RefCountedThing::s_iInstances == 2);

	RefCountedPtr<RefCountedThing> a(rawA);
	RefCountedPtr<RefCountedThing> b(rawB);

	a = b;
	Assert((RefCountedThing*)a == rawB);
	Assert(rawB->GetRefCount() == 2);

	// a's original reference to rawA was released, freeing it
	Assert(RefCountedThing::s_iInstances == 1);
}

Fact("RefCountedPtr Move Constructor Does Not AddRef")
{
	RefCountedThing::s_iInstances = 0;
	RefCountedThing* raw = new RefCountedThing();

	RefCountedPtr<RefCountedThing> a(raw);
	RefCountedPtr<RefCountedThing> b(move(a));

	Assert(raw->GetRefCount() == 1);
	Assert((RefCountedThing*)a == nullptr);
	Assert((RefCountedThing*)b == raw);
	Assert(RefCountedThing::s_iInstances == 1);
}

Fact("RefCountedPtr Move Assignment Releases Old")
{
	RefCountedThing::s_iInstances = 0;
	RefCountedThing* rawA = new RefCountedThing();
	RefCountedThing* rawB = new RefCountedThing();

	RefCountedPtr<RefCountedThing> a(rawA);
	RefCountedPtr<RefCountedThing> b(rawB);

	a = move(b);
	Assert((RefCountedThing*)a == rawB);
	Assert(rawB->GetRefCount() == 1);
	Assert((RefCountedThing*)b == nullptr);

	// a's original reference to rawA was released, freeing it
	Assert(RefCountedThing::s_iInstances == 1);
}

Fact("RefCountedPtr Assign From Raw Pointer Adopts Without AddRef")
{
	RefCountedThing::s_iInstances = 0;
	RefCountedThing* rawA = new RefCountedThing();
	RefCountedThing* rawB = new RefCountedThing();

	RefCountedPtr<RefCountedThing> p(rawA);
	p = rawB;

	Assert((RefCountedThing*)p == rawB);
	Assert(rawB->GetRefCount() == 1);

	// rawA's only reference was released, freeing it
	Assert(RefCountedThing::s_iInstances == 1);
}

Fact("RefCountedPtr Detach Releases Ownership Without Calling Release")
{
	RefCountedThing::s_iInstances = 0;
	RefCountedThing* raw = new RefCountedThing();

	RefCountedPtr<RefCountedThing> p(raw);
	RefCountedThing* detached = p.Detach();

	Assert(detached == raw);
	Assert((RefCountedThing*)p == nullptr);
	Assert(raw->GetRefCount() == 1);
	Assert(RefCountedThing::s_iInstances == 1);

	detached->Release();
	Assert(RefCountedThing::s_iInstances == 0);
}

Fact("RefCountedPtr Arrow And Dereference Operators")
{
	RefCountedThing::s_iInstances = 0;
	RefCountedPtr<RefCountedThing> p(new RefCountedThing());

	Assert(p->GetRefCount() == 1);
	Assert((*p).GetRefCount() == 1);
}

Fact("RefCountedPtr Address Of Operator For Out Params")
{
	RefCountedThing::s_iInstances = 0;

	RefCountedPtr<RefCountedThing> p;

	// Simulate a COM-style out-param function (eg: CoCreateInstance) that
	// fills in the raw pointer directly, already holding a reference for us
	RefCountedThing** ppOut = &p;
	*ppOut = new RefCountedThing();

	Assert((RefCountedThing*)p != nullptr);
	Assert(p->GetRefCount() == 1);
	Assert(RefCountedThing::s_iInstances == 1);
}

Fact("RefCountedPtr Bool Operator")
{
	RefCountedThing::s_iInstances = 0;

	RefCountedPtr<RefCountedThing> empty;
	RefCountedPtr<RefCountedThing> nonEmpty(new RefCountedThing());

	Assert(!empty);
	Assert((bool)nonEmpty);

	nonEmpty = nullptr;
	Assert(!nonEmpty);
	Assert(RefCountedThing::s_iInstances == 0);
}

Fact("List Of RefCountedPtr")
{
	RefCountedThing::s_iInstances = 0;

	List<RefCountedPtr<RefCountedThing>> list;
	list.Add(new RefCountedThing());
	list.Add(new RefCountedThing());
	list.Add(new RefCountedThing());
	Assert(RefCountedThing::s_iInstances == 3);

	{
		// Holding a copy keeps the instance alive after removal from the list
		RefCountedPtr<RefCountedThing> kept = list[0];
		Assert(kept->GetRefCount() == 2);
		list.RemoveAt(0);
		Assert(RefCountedThing::s_iInstances == 3);
	}
	Assert(RefCountedThing::s_iInstances == 2);

	list.Clear();
	Assert(RefCountedThing::s_iInstances == 0);
}
