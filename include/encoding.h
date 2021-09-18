#ifndef __simplelib_encoding_h__
#define __simplelib_encoding_h__

#include "semantics.h"
#include "string.h"

namespace SimpleLib
{

template <typename TFrom, typename TTo>
struct Encoding
{
};

template <typename T>
struct PassthroughEncoding
{
	void Process(T in, IStringWriter<T>& out)
	{
		out.Write(in);
	}
};

template<> struct Encoding<char, char> : PassthroughEncoding<char> {};
template<> struct Encoding<char16_t, char16_t> : PassthroughEncoding<char16_t> {};
template<> struct Encoding<char32_t, char32_t> : PassthroughEncoding<char32_t> {};
template<> struct Encoding<wchar_t, wchar_t> : PassthroughEncoding<wchar_t> {};


template<>
struct Encoding<char, char32_t>
{
	Encoding()
	{
		_pending = 0;
		_pendingCount = 0;
	}

	char32_t _pending;
	int _pendingCount;

	void Process(char in, IStringWriter<char32_t>& out)
	{
		if (_pendingCount == 0)
		{
			if ((in & 0x80) == 0)
			{
				// Single byte
				out.Write(in);
				return;
			}

			if ((in & 0xE0) == 0xC0)
			{
				// Double byte
				_pending = in & 0x1f;
				_pendingCount = 1;
				return;
			}

			if ((in & 0xF0) == 0xE0)
			{
				// Triple byte
				_pending = in & 0x0f;
				_pendingCount = 2;
				return;
			}

			if ((in & 0xF8) == 0xF0)
			{
				// Quad byte
				_pending = in & 0x07;
				_pendingCount = 3;
				return;
			}
		}
		else
		{
			_pending = (_pending << 6) | (in & 0x3f);
			_pendingCount--;
			if (_pendingCount == 0)
			{
				out.Write(_pending);
			}
		}
	}
};

template<>
struct Encoding<char32_t, char16_t>
{
	void Process(char32_t in, IStringWriter<char16_t>& out)
	{
		if (in >= 0x10000)
		{
			out.Write(0xD800 | (((in - 0x10000) >> 10) & 0x3FF));
			out.Write(0xDC00 | ((in - 0x10000) & 0x3ff));
		}
		else
		{
			out.Write((char16_t)in);
		}
	}
};

template<>
struct Encoding<char32_t, char>
{
	void Process(char32_t in, IStringWriter<char>& out)
	{
		if (in < 0x80)
		{
			out.Write(in & 0x7f);
			return;
		}
		if (in < 0x800)
		{
			out.Write( 0xC0 | ((in >> 6) & 0x1f));
			out.Write( 0x80 | (in & 0x3f));
			return;
		}
		if (in < 0x10000)
		{
			out.Write( 0xE0 | ((in >> 12) & 0x0F));
			out.Write( 0x80 | ((in >> 6) & 0x3F));
			out.Write( 0x80 | (in & 0x3f));
			return;
		}

		out.Write( 0xF0 | ((in >> 18) & 0x07));
		out.Write( 0x80 | ((in >> 12) & 0x3F));
		out.Write( 0x80 | ((in >> 6) & 0x3F));
		out.Write( 0x80 | (in & 0x3f));
	}
};

template<>
struct Encoding<char, char16_t> : IStringWriter<char32_t>
{
	Encoding<char, char32_t> _to32;
	Encoding<char32_t, char16_t> _to16;
	IStringWriter<char16_t>* _out;

	void Process(char in, IStringWriter<char16_t>& out)
	{
		_out = &out;
		_to32.Process(in, *this);
	}

	virtual void Write(char32_t ch) override
	{
		_to16.Process(ch, *_out);
	}
};

template<>
struct Encoding<char16_t, char32_t>
{
	Encoding()
	{
		_pendingHighSurrogate = 0;
	}

	char16_t _pendingHighSurrogate;

	void Process(char16_t in, IStringWriter<char32_t>& out)
	{
		if (in >= 0xD800 && in < 0xDC00)
		{
			// high surrogate
			_pendingHighSurrogate = in;
		}
		else if (in >= 0xDC00 && in < 0xE000)
		{
			// low surrogate
			out.Write(
				0x10000 + 
				(((((char32_t)_pendingHighSurrogate) & 0x3FF) << 10) | 
				(((char32_t)in) & 0x3FF))
				);
			_pendingHighSurrogate = 0;
		}
		else
		{
			_pendingHighSurrogate = 0;
			out.Write((char32_t)in);
		}
	}
};

template<>
struct Encoding<char16_t, char> : IStringWriter<char32_t>
{
	Encoding<char16_t, char32_t> _to32;
	Encoding<char32_t, char> _to8;
	IStringWriter<char>* _out;

	void Process(char16_t in, IStringWriter<char>& out)
	{
		_out = &out;
		_to32.Process(in, *this);
	}

	virtual void Write(char32_t ch) override
	{
		_to8.Process(ch, *_out);
	}
};


template<>
struct Encoding<char, wchar_t>
{
	Encoding<char, char16_t> actual;

	void Process(char in, IStringWriter<wchar_t>& out)
	{
		actual.Process(in, reinterpret_cast<IStringWriter<char16_t>&>(out));
	}
};

template<>
struct Encoding<wchar_t, char>
{
	Encoding<char16_t, char> actual;

	void Process(wchar_t in, IStringWriter<char>& out)
	{
		actual.Process(in, out);
	}
};

template <typename TTo, typename TFrom>
CCoreString<TTo> Encode(const TFrom* in)
{
	if (!in)
		return (TTo*)nullptr;
		
	CCoreStringBuilder<TTo> out;
	Encoding<TFrom, TTo> enc;
	while (*in)
	{
		enc.Process(*in++, out);
	}
	return out.ToString();
}


} // namespace

#endif  // __simplelib_encoding_h__