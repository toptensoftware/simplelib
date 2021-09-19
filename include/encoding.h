#ifndef __simplelib_encoding_h__
#define __simplelib_encoding_h__

#include "semantics.h"
#include "string.h"

namespace SimpleLib
{

struct CEncoding
{
	static char32_t RecoverUtf8(const char*& p)
	{
		// Skip the problematic character
		p++;

		// Find the next lead byte
		while (true)
		{
			char ch = *p;
			if ((ch & 0x80) == 0 || (ch & 0xE0) == 0xC0 || (ch & 0xF0) == 0xE0 || (ch & 0xF8) == 0xF0)
				return -1;
			p++;
		}
	}

	// Read a utf32 character from a utf8 string
	static char32_t DecodeUtf8(const char*& p)
	{
		char32_t ch = *p;

		if ((ch & 0x80) == 0)
		{
			// 1-byte character
			p++;
			return ch;
		}

		if ((ch & 0xE0) == 0xC0)
		{
			// 2-byte character
			char32_t ch2 = p[1];

			// Check valid
			if ((ch2 & 0xc0) != 0x80 || (ch2 & 0x1f) == 0)
				return RecoverUtf8(p);

			// Done
			p+=2;
			return ((ch & 0x1f) << 6) | (ch2 & 0x3f);
		}

		if ((ch & 0xF0) == 0xE0)
		{
			// 3-byte character

			char32_t ch2 = p[1];
			if ((ch2 & 0xc0) != 0x80 || (ch2 & 0x0f) == 0)
				return RecoverUtf8(p);

			char32_t ch3 = p[2];
			if ((ch3 & 0xc0) != 0x80)
				return RecoverUtf8(p);

			ch = ((ch & 0x0f) << 12) | ((ch2 & 0x3f) << 6) | (ch3 & 0x3F);

			if ((ch & 0xD800) == 0xD800)
				return RecoverUtf8(p);

			p+=3;
			return ch;
		}

		if ((ch & 0xF8) == 0xF0)
		{
			// 4-byte character

			char32_t ch2 = p[1];
			if ((ch2 & 0xc0) != 0x80 && (ch & 0x07) == 0)
				return RecoverUtf8(p);

			char32_t ch3 = p[2];
			if ((ch3 & 0xc0) != 0x80)
				return RecoverUtf8(p);

			char32_t ch4 = p[3];
			if ((ch3 & 0xc0) != 0x80)
				return RecoverUtf8(p);

			ch = ((ch & 0x07) << 18) | ((ch2 & 0x3f) << 12) | 
					((ch3 & 0x3F) << 6) | (ch4 & 0x3f);

			if (ch >= 0x110000)
				return RecoverUtf8(p);

			p+=4;
			return ch;
		}

		return RecoverUtf8(p);
	}

	// Write a utf32 character to a utf8 stream
	static bool EncodeUtf8(char*& p, char32_t ch)
	{
		if (ch < 0x80)
		{
			*p++ = ch & 0x7f;
			return true;
		}
		if (ch < 0x800)
		{
			*p++ = 0xC0 | ((ch >> 6) & 0x1f);
			*p++ = 0x80 | (ch & 0x3f);
			return true;
		}

		// Can't encode utf16 surrogate pairs
		if ((ch & 0xD800) == 0xD800)
			return false;

		if (ch < 0x10000)
		{
			*p++ = 0xE0 | ((ch >> 12) & 0x0F);
			*p++ = 0x80 | ((ch >> 6) & 0x3F);
			*p++ = 0x80 | (ch & 0x3f);
			return true;
		}

		if (ch < 0x110000)
		{
			*p++ = 0xF0 | ((ch >> 18) & 0x07);
			*p++ = 0x80 | ((ch >> 12) & 0x3F);
			*p++ = 0x80 | ((ch >> 6) & 0x3F);
			*p++ = 0x80 | (ch & 0x3f);
			return true;
		}

		return false;
	}

	// Read a utf32 character from a utf16 stream
	static char32_t DecodeUtf16(const char16_t*& p)
	{
		// Fast path for non-surrogate pairs
		if ((p[0] & 0xD800) != 0xD800)
		{
			return *p++;
		}

		// High surrogate
		if ((p[0] & 0xDC00) == 0xD800)
		{
			// Low surrogate
			if ((p[1] & 0xDC00) == 0xDC00)
			{
				// Pack
				char32_t ch = 0x10000 + (char32_t((p[0] & 0x3FF) << 10) | char32_t(p[1] & 0x3FF));
				p += 2;

				if ((ch & 0xD800) == 0xD800)
				{
					// Surrogate character encoded as surrogates
					return -1;
				}
				else
					return ch;
			}
			else
			{
				// High surrogate without low
				p++;
				return -1;
			}
		}

		// Low surrogate without high
		p++;
		return -1;
	}

	// Write a utf32 character to a utf16 stream
	static bool EncodeUtf16(char16_t*& p, char32_t ch)
	{
		if (ch < 0x10000)
		{
			if ((ch & 0xD800) == 0xD800)
				return false;

			*p++ = (char16_t)ch;
			return true;
		}

		if (ch < 0x110000)
		{
			ch -= 0x10000;
			*p++ = 0xD800 | ((ch >> 10) & 0x3FF);
			*p++ = 0xDC00 | (ch & 0x3ff);
			return true;
		}

		return false;
	}

};




	template <typename TTo, typename TFrom>
	CCoreString<TTo> Convert(const TFrom* p)
	{
		return (TTo*)nullptr;
	}

	template <>
	CCoreString<char32_t> Convert<char32_t, char>(const char* p)
	{
		CCoreStringBuilder<char32_t> sb;
		while (*p)
		{
			char32_t ch = CEncoding::DecodeUtf8(p);
			if (ch == (char32_t)-1)
				ch = 0xFFFD;
			sb.Write(ch);
		}
		return sb.ToString();
	}

	template <>
	CCoreString<char16_t> Convert<char16_t, char>(const char* p)
	{
		CCoreStringBuilder<char16_t> sb;
		char16_t ch16[2];
		while (*p)
		{
			// 8 -> 32
			char32_t ch = CEncoding::DecodeUtf8(p);
			if (ch == (char32_t)-1)
				ch = 0xFFFD;
			
			// 32 -> 16
			char16_t* p = ch16;
			CEncoding::EncodeUtf16(p, ch);
			
			// Write
			sb.Write(ch16[0]);
			if (p == &ch16[2])
				sb.Write(ch16[1]);
		}

		return sb.ToString();
	}

	template <>
	CCoreString<char> Convert<char, char32_t>(const char32_t* p)
	{
		// Start by encoding to local buffer
		char szBuf[1024];
		char* out = szBuf;
		while (*p && out < szBuf + sizeof(szBuf) - 5)
		{
			CEncoding::EncodeUtf8(out, *p++);
		}

		// All converted
		if (*p == 0)
		{
			*out = '\0';
			return szBuf;
		}

		// Need a string builder
		CCoreStringBuilder<char> sb;
		while (*p)
		{
			if (out >= szBuf + sizeof(szBuf) - 5)
			{
				*out = '\0';
				sb.Append(szBuf);
				out = szBuf;
			}
			CEncoding::EncodeUtf8(out, *p++);
		}

		// Final part
		*out = '\0';
		sb.Append(szBuf);

		return sb.ToString();
	}

	template <>
	CCoreString<char> Convert<char, char16_t>(const char16_t* p)
	{
		// Start by encoding to local buffer
		char szBuf[1024];
		char* out = szBuf;
		while (*p && out < szBuf + sizeof(szBuf) - 5)
		{
			CEncoding::EncodeUtf8(out, CEncoding::DecodeUtf16(p));
		}

		// All converted
		if (*p == 0)
		{
			*out++ = '\0';
			return szBuf;
		}

		// Need a string builder
		CCoreStringBuilder<char> sb;
		while (*p)
		{
			if (out >= szBuf + sizeof(szBuf) - 5)
			{
				*out = '\0';
				sb.Append(szBuf);
				out = szBuf;
			}
			CEncoding::EncodeUtf8(out, CEncoding::DecodeUtf16(p));
		}

		// Final part
		*out = '\0';
		sb.Append(szBuf);

		return sb.ToString();
	}


/*
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
*/

} // namespace

#endif  // __simplelib_encoding_h__