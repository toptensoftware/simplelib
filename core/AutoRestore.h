#pragma once

namespace SimpleLib
{

// Captures the value of a variable on construction and restores it
// when going out of scope.  Handy for temporarily overriding a flag
// or counter for the duration of a scope.
template <class T>
class AutoRestore
{
public:
	// Capture the current value, leaving the variable unchanged.
	explicit AutoRestore(T& var) :
		m_var(var),
		m_oldValue(var)
	{
	}

	// Capture the current value and immediately assign a new one.
	AutoRestore(T& var, const T& newValue) :
		m_var(var),
		m_oldValue(var)
	{
		m_var = newValue;
	}

	~AutoRestore()
	{
		m_var = m_oldValue;
	}

	// Not copyable or movable - it owns a reference to a stack variable
	// and must be released exactly once, in the scope that created it.
	AutoRestore(const AutoRestore&) = delete;
	AutoRestore(AutoRestore&&) = delete;
	AutoRestore& operator=(const AutoRestore&) = delete;
	AutoRestore& operator=(AutoRestore&&) = delete;

	// The originally captured value.
	const T& OldValue() const
	{
		return m_oldValue;
	}

private:
	T& m_var;
	T m_oldValue;
};


}
