#ifndef __simplelib_stringops_h__
#define __simplelib_stringops_h__

#include "codepointiterator.h"
#include "caseconversion.h"

namespace SimpleLib
{

struct ICodePointWriter
{
	virtual void Write(char32_t ch) = 0;
};

struct ICodePointSplitWriter
{
	virtual void EnterSplit() = 0;
	virtual void LeaveSplit() = 0;
	virtual void Write(char32_t ch) = 0;
}

struct SStringOps
{
	template<typename T>
	static void ToUpper(ICodePointWriter& w, T a)
	{
		a.MoveTo(0);
		while (true)
		{
			char32_t ch = SCaseConversion::ToUpper(a.GetCodePoint());
			if (ch == 0)
				return;
			w.Write(ch);
		}
	}

	template<typename T>
	static void ToLower(ICodePointWriter& w, T a)
	{
		a.MoveTo(0);
		while (true)
		{
			char32_t ch = SCaseConversion::ToLower(a.GetCodePoint());
			if (ch == 0)
				return;
			w.Write(ch);
		}
	}

	template<typename T>
	static void SubString(ICodePointWriter& w, T a, int start, int length)
	{
		// Move to start position
		if (start < 0)
		{
			a.MoveToEnd();
			if (!w.Rewind(-start))
				return;
		}
		else
		{
			if (!a.MoveTo(start))
				return;
		}

		// Extract string
		for (int i=0; i<length; i++)
		{
			w.Write(a.GetCodePoint());
			if (!a.Advance(1))
				return;
		}
	}

	int IndexOf(chars);
	int IndexOf(substring);
	int LastIndexOf(chars);
	int LastIndexOf(substring);
	void Replace()
	bool IsEqualTo();
	Join();
	Split();


	template<typename S = SCase, typename T1, typename T2>
	static bool StartsWith(T1& a, T2& b)
	{
		a.MoveTo(0);
		b.MoveTo(0);

		while (b.GetCodePoint() != '\0')
		{
			if (S::TCompare::Compare(a.GetCodePoint(), b.GetCodePoint()) != 0)
				return false;
			a.Advance(1);
			b.Advance(1);
		}

		return true;
	}

	template<typename S = SCase, typename T1, typename T2>
	static bool EndsWith(T1& a, T2& b)
	{
		if (!a.MoveTo(a.GetCodePointCount() - b.GetCodePointCount()))
			return false;
		b.MoveTo(0);

		while (b.GetCodePoint() != '\0')
		{
			if (S::TCompare::Compare(a.GetCodePoint(), b.GetCodePoint()) != 0)
				return false;
			a.Advance(1);
			b.Advance(1);
		}

		return true;
	}
};

} // namespace

#endif  // __simplelib_codepointiterator_h__