#pragma once

namespace SimpleLib
{

class SlimLock
{
public:
	SlimLock()
	{
    	InitializeSRWLock(&m_slimlock);
	}

	virtual ~SlimLock()
	{
	}

	void EnterExclusive()
	{
    	AcquireSRWLockExclusive(&m_slimlock);
	}
	bool TryEnterExclusive()
	{
        return TryAcquireSRWLockExclusive(&m_slimlock);
	}
	void LeaveExclusive()
	{
        ReleaseSRWLockExclusive(&m_slimlock);
	}
	void EnterShared()
	{
    	AcquireSRWLockShared(&m_slimlock);
	}
	bool TryEnterShared()
	{
    	return TryAcquireSRWLockShared(&m_slimlock);
	}
	void LeaveShared()
	{
        ReleaseSRWLockShared(&m_slimlock);
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