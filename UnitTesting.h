#pragma once

#include "./Core/String.h"

namespace SimpleLib
{

class TestEntry;

enum class TestEntryKind
{
	Initialize,
	Fact,
	Deinitialize,
};

class TestRunner
{
public:
	static void Run();

	static void Register(TestEntry* p)
	{
		m_entries.Add(p);
	}

private:
	static void RunKind(TestEntryKind kind);
	inline static List<TestEntry*> m_entries;
	friend class TestEntry;
};

class TestEntry
{
public:
	TestEntry(TestEntryKind kind, const char* name, const char* file, int line, void (*fn)())
	{
		m_kind = kind;
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

	TestEntryKind m_kind;
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


inline void TestRunner::RunKind(TestEntryKind kind)
{
	for (int i=0; i<m_entries.GetCount(); i++)
	{
		TestEntry* f = m_entries[i];
		if (f->m_kind != kind)
			continue;

		m_entries[i]->m_fn();
	}

}

inline void TestRunner::Run()
{
	RunKind(TestEntryKind::Initialize);

	int failed = 0;
	int count = 0;
	for (int i=0; i<m_entries.GetCount(); i++)
	{
		TestEntry* f = m_entries[i];
		if (f->m_kind != TestEntryKind::Fact)
			continue;

		printf("%s: ", f->m_strName.sz());
		fflush(stdout);
        try
        {
			count++;
    		m_entries[i]->m_fn();
			printf("ok\n");
        }
        catch (const AssertionFailed& e)
        {
            fprintf(stderr, "assertion failed: %s\n  at %s(%i)\n", e.Expression, e.File, e.Line);
			failed++;
        }
	}

	RunKind(TestEntryKind::Deinitialize);

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

#define Initialize() \
static void SIMPLELIB_UNIQUE_NAME(fn)(); \
static SimpleLib::TestEntry SIMPLELIB_UNIQUE_NAME(fe)(TestEntryKind::Initialize, nullptr, __FILE__, __LINE__, &SIMPLELIB_UNIQUE_NAME(fn)); \
static void SIMPLELIB_UNIQUE_NAME(fn)() \

#define Deinitialize() \
static void SIMPLELIB_UNIQUE_NAME(fn)(); \
static SimpleLib::TestEntry SIMPLELIB_UNIQUE_NAME(fe)(TestEntryKind::Deinitialize, nullptr, __FILE__, __LINE__, &SIMPLELIB_UNIQUE_NAME(fn)); \
static void SIMPLELIB_UNIQUE_NAME(fn)() \

#define Fact(name) \
static void SIMPLELIB_UNIQUE_NAME(fn)(); \
static SimpleLib::TestEntry SIMPLELIB_UNIQUE_NAME(fe)(TestEntryKind::Fact, name, __FILE__, __LINE__, &SIMPLELIB_UNIQUE_NAME(fn)); \
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