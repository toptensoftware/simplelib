#pragma once

#include "../Core/String.h"

namespace SimpleLib
{

class FactEntry;

class TestRunner
{
public:
	static void Run();

	static void Register(FactEntry* p)
	{
		m_allFacts.Add(p);
	}

private:
	inline static List<FactEntry*> m_allFacts;
	friend class FactEntry;
};

class FactEntry
{
public:
	FactEntry(const char* name, const char* file, int line, void (*fn)())
	{
		m_strName = name;
		m_strFile = file;
		m_iLine = line;
		m_fn = fn;
		TestRunner::Register(this);
	}

	void SetFn(void (*fn)())
	{
		m_fn = fn;
	}

	String m_strName;
	String m_strFile;
	int m_iLine;
	void (*m_fn)();

};

class AssertionFailed
{
public:
	AssertionFailed(const char* expr, const char* file, int line)
	{
		Expression = expr;
		File = file;
		Line = line;
	}

	const char* Expression;
	const char* File;
	int Line;
};


inline void TestRunner::Run()
{
	int failed = 0;
	int count = 0;
	for (int i=0; i<m_allFacts.GetCount(); i++)
	{
		FactEntry* f = m_allFacts[i];
		printf("%s: ", f->m_strName.sz());
        try
        {
			count++;
    		m_allFacts[i]->m_fn();
			printf("ok\n");
        }
        catch (const AssertionFailed& e)
        {
            fprintf(stderr, "assertion failed: %s\n  at %s(%i)\n", e.Expression, e.File, e.Line);
			failed++;
        }
	}

	if (failed == 0)
		printf("\nAll (%i) tests passed.\n", count);
	else
		printf("\n%i of %i tests failed.\n", failed, count);
}


#ifndef STRINGIZE
#define STRINGIZE_(x) #x
#define STRINGIZE(x) STRINGIZE_(x)
#endif

#ifndef CONCAT
#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#endif

// Deliberately not named UNIQUE_NAME: that name collides with a macro
// pulled in by <Windows.h> (um/nb30.h), which silently clobbers ours
// whenever a test file includes both this header and Windows.h (directly
// or via Threading.h) - whichever is included last wins, breaking every
// Fact() after that point in the translation unit.
#define SIMPLELIB_UNIQUE_NAME(prefix) CONCAT(prefix, __LINE__)

#define Fact(name) \
static void SIMPLELIB_UNIQUE_NAME(fn)(); \
static SimpleLib::FactEntry SIMPLELIB_UNIQUE_NAME(fe)(name, __FILE__, __LINE__, &SIMPLELIB_UNIQUE_NAME(fn)); \
static void SIMPLELIB_UNIQUE_NAME(fn)() \

inline void _Assert(bool value, const char* expr, const char* file, int line)
{
	if (!value)
	{
		throw AssertionFailed(expr, file, line);
	}
}

#define Assert(x) \
	_Assert(x, #x, __FILE__, __LINE__)

}