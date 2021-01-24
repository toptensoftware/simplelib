#ifndef _test_core_h_
#define _test_core_h_

#include <stdio.h>
#include <string.h>

#include "stringpool.h"
#include "formatting.h"

/*
eg:

void mytest_with(int a, int b, int c)
{
	assert_equal(a + b, c);
}

void mytest()
{
	run(mytest_with(5, 5, 10));
	run(mytest_with(10, 10, 20));
	run(mytest_with(100, 100, 0));
}

int main()
{
	run(mytest());
	test_summary();
}

-------- output --------

Test mytest()
  Test mytest_with(5, 5, 10) - PASS
  Test mytest_with(10, 10, 20) - PASS
  Test mytest_with(100, 100, 0)

				  failed: a + b should equal c
				   a + b: 200
					   c: 0
					  at: C:\Users\Brad\source\repos\simplelibsandbox\main.cpp(7)


Failed 1 of 4 tests!

*/


#define run(x) \
	SimpleLib::CUnitTesting::EnterTest(#x); \
	x; \
	SimpleLib::CUnitTesting::LeaveTest()

#undef assert

#define assert_condition(a, compare, msg) { auto _a = a; if (!(compare)) { SimpleLib::CUnitTesting::FailTest(__FILE__, __LINE__, #a " " msg, #a, test_format(_a)); return; } }
#define assert_compare(a, b, compare, msg) { auto _a = a; auto _b = b;  if (!(compare)) { SimpleLib::CUnitTesting::FailTest(__FILE__, __LINE__, #a " " msg " " #b, #a, test_format(_a), #b, test_format(_b) ); return; } }

#define assert_true(a) assert_condition(a, !!_a, "should be true")
#define assert_false(a) assert_condition(a, !_a, "should be false")
#define assert_null(a) assert_condition(a, _a == nullptr, "should be null")
#define assert_not_null(a) assert_condition(a, _a != nullptr, "should not be null")
#define assert_error(a) assert_condition(a, _a != 0, "should have failed with error")
#define assert_not_error(a) assert_condition(a, _a == 0, "should not have failed with error")

#define assert_equal(a, b) assert_compare(a, b, _a == _b, "should equal")
#define assert_not_equal(a, b) assert_compare(a, b, _a != _b, "should not equal")
#define assert_string_equal(a, b) assert_compare(a, b, SimpleLib::CUnitTesting::StringCompare(_a,_b)==0, "should equal")
#define assert_string_not_equal(a, b) assert_compare(a, b, SimpleLib::CUnitTesting::StringCompare(_a,_b)!=0, "should not equal")

#define test_summary() SimpleLib::CUnitTesting::WriteSummary()


namespace SimpleLib
{

class CUnitTesting
{
public:
	static void EnterTest(const char* name)
	{
		ActiveTest() = new CTest(ActiveTest(), name);
	}

	static void LeaveTest()
	{
		ActiveTest()->Finish();
		ActiveTest() = ActiveTest()->GetParent();
		StringPool().FreeAll();
	}

	static void FailTest(const char* file, int lineNumber,
		const char* message,
		const char* exprA = nullptr, const char* valA = nullptr,
		const char* exprB = nullptr, const char* valB = nullptr
	)
	{
		if (ActiveTest())
			ActiveTest()->Fail(file, lineNumber, message, exprA, valA, exprB, valB);
	}

	static void WriteSummary()
	{
		if (FailedTests() == 0)
		{
			printf("\nPassed all %i tests!\n\n", TotalTests());
		}
		else
		{
			printf("\nFailed %i of %i tests!\n\n", FailedTests(), TotalTests());
		}
	}


	static const char* vsprintf(const char* format, va_list args)
	{
		CFormatBuilder<char> buf;
		buf.FormatV(format, args);
		return StringPool().Alloc(buf);
	}

	static const char* FormatString(const char* value, char chDelim = '\"')
	{
		if (value == nullptr)
			return "nullptr";

		CStringBuilder<char> buf;
		buf.Append(chDelim);

		const char* p = value;
		while (*p)
		{
			switch (*p)
			{
			case '\0': buf.Append("\\0", 2); break;
			case '\a': buf.Append("\\a", 2); break;
			case '\b': buf.Append("\\b", 2); break;
			case '\f': buf.Append("\\f", 2); break;
			case '\n': buf.Append("\\n", 2); break;
			case '\r': buf.Append("\\r", 2); break;
			case '\t': buf.Append("\\t", 2); break;
			case '\v': buf.Append("\\v", 2); break;
			case '\\': buf.Append("\\\\", 2); break;
			case '\'': buf.Append("\\\'", 2); break;
			case '\"': buf.Append("\\\"", 2); break;

			default:
				buf.Append(*p);
				break;
			}

			p++;
		}

		buf.Append(chDelim);
		return StringPool().Alloc(buf);
	}

	static int StringCompare(const char* a, const char* b)
	{
		if (a == nullptr && b == nullptr)
			return 0;
		if (a == nullptr)
			return -1;
		if (b == nullptr)
			return 1;
		return strcmp(a, b);
	}

	static CStringPool<char>& StringPool() { static CStringPool<char> val; return val; }

private:
	class CTest;
	static int& TotalTests() { static int val = 0; return val; }
	static int& FailedTests() { static int val = 0; return val; }
	static CTest*& ActiveTest() { static CTest* val = nullptr; return val; }

	class CTest
	{
	public:
		CTest(CTest* pParent, const char* name)
		{
			TotalTests()++;

			if (pParent)
			{
				pParent->CloseHeader();
				m_iDepth = pParent->m_iDepth + 1;
			}
			else
			{
				m_iDepth = 0;
			}
			m_pParent = pParent;
			m_bHeaderClosed = false;

			WriteIndent();
			printf("Test %s", name);

		}

		void WriteIndent()
		{
			for (int i = 0; i < 2 * m_iDepth; i++)
			{
				fputc(' ', stdout);
			}
		}

		void CloseHeader()
		{
			if (!m_bHeaderClosed)
			{
				printf("\n");
				m_bHeaderClosed = true;
			}
		}

		void Finish()
		{
			if (!m_bHeaderClosed)
			{
				printf(" - ");
				printf("PASS");
				CloseHeader();
			}
		}

		void Fail(
			const char* file, int lineNumber,
			const char* message,
			const char* exprA, const char* valA,
			const char* exprB, const char* valB
		)
		{
			if (m_pParent)
				m_pParent->ChildFailed();

			CloseHeader();


			printf("\n");
			m_iDepth++;
			WriteIndent();
			printf("%20s: %s\n", "failed", message);
			if (exprA && strcmp(exprA, valA))
			{
				WriteIndent();
				printf("%20s: %s\n", exprA, valA);
			}
			if (exprB && strcmp(exprB, valB))
			{
				WriteIndent();
				printf("%20s: %s\n", exprB, valB);
			}
			WriteIndent();
			printf("%20s: %s(%i)\n", "at", file, lineNumber);
			m_iDepth--;
			printf("\n");

			FailedTests()++;
		}

		void ChildFailed()
		{
			CloseHeader();
		}

		CTest* GetParent() { return m_pParent; }

		CTest* m_pParent;
		int m_iDepth;
		bool m_bHeaderClosed;
	};
};

}	// namespace






inline const char* test_sprintf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	const char* result = SimpleLib::CUnitTesting::vsprintf(format, args);
	va_end(args);
	return result;
}


inline const char* test_format(bool value)
{
	return value ? "true" : "false";
}

inline const char* test_format(int value)
{
	return test_sprintf("%i", value);
}

inline const char* test_format(unsigned int value)
{
	return test_sprintf("%u", value);
}

inline const char* test_format(long value)
{
	return test_sprintf("%li", value);
}

inline const char* test_format(unsigned long value)
{
	return test_sprintf("%lu", value);
}

inline const char* test_format(long long value)
{
	return test_sprintf("%lli", value);
}

inline const char* test_format(unsigned long long value)
{
	return test_sprintf("%llu", value);
}

inline const char* test_format(void* value)
{
	return test_sprintf("0x%p", value);
}

inline const char* test_format(char value)
{
	char sz[2] = "x";
	sz[0] = value;
	return SimpleLib::CUnitTesting::FormatString(sz, '\'');
}

inline const char* test_format(double value)
{
	return test_sprintf("%f", value);
}

inline const char* test_format(const char* value)
{
	return SimpleLib::CUnitTesting::FormatString(value);
}



#endif // _test_core_h_