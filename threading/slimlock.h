#pragma once

namespace SimpleLib
{

class SlimLock
{
public:
	SlimLock()
	{
		Platform::slimlockCreate(m_slimlock);
	}

	virtual ~SlimLock()
	{
		Platform::slimlockDestroy(m_slimlock);
	}

	void EnterExclusive()
	{
		Platform::slimlockEnterExclusive(m_slimlock);
	}
	bool TryEnterExclusive()
	{
		return Platform::slimlockTryEnterExclusive(m_slimlock);
	}
	void LeaveExclusive()
	{
		Platform::slimlockLeaveExclusive(m_slimlock);
	}
	void EnterShared()
	{
		Platform::slimlockEnterShared(m_slimlock);
	}
	bool TryEnterShared()
	{
		return Platform::slimlockTryEnterShared(m_slimlock);
	}
	void LeaveShared()
	{
		Platform::slimlockLeaveShared(m_slimlock);
	}

    void Enter(bool exclusive)
    {
        if (exclusive)
            EnterExclusive();
        else
            EnterShared();
    }

    bool TryEnter(bool exclusive)
    {
        if (exclusive)
            return TryEnterExclusive();
        else
            return TryEnterShared();
    }

    void Leave(bool exclusive)
    {
        if (exclusive)
            LeaveExclusive();
        else
            LeaveShared();
    }

	SRWLOCK m_slimlock;
};

class EnterSlimLock
{
public:
	EnterSlimLock()
	{
		_slimLock = nullptr;
	}

	EnterSlimLock(SlimLock& slimlock, bool exclusive)
	{
		_slimLock = nullptr;
		Enter(slimlock, exclusive);
	}

	~EnterSlimLock()
	{
		if (_slimLock != NULL)
			Leave();
	}

	void Enter(SlimLock& slimlock, bool exclusive)
	{
		assert(_slimLock == NULL);
		_slimLock = &slimlock;
        _exclusive = exclusive;
		_slimLock->Enter(exclusive);
	}

	bool TryEnter(SlimLock& slimlock, bool exclusive)
	{
		assert(_slimLock == NULL);
		if (slimlock.TryEnter(exclusive))
		{
			_slimLock = &slimlock;
            _exclusive = exclusive;
			return true;
		}
		return false;
	}

	void Leave()
	{
		assert(_slimLock != NULL);
		_slimLock->Leave(_exclusive);
		_slimLock = NULL;
	}

	SlimLock* _slimLock;
    bool _exclusive;
};




}