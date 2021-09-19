#ifndef __simplelib_encoding_h__
#define __simplelib_encoding_h__

#include "semantics.h"
#include "string.h"

namespace SimpleLib
{

struct CEncodingUtf8
{
	static char32_t Recover(const char*& p)
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
	static char32_t Decode(const char*& p)
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
				return Recover(p);

			// Done
			p+=2;
			return ((ch & 0x1f) << 6) | (ch2 & 0x3f);
		}

		if ((ch & 0xF0) == 0xE0)
		{
			// 3-byte character

			char32_t ch2 = p[1];
			if ((ch2 & 0xc0) != 0x80 || (ch2 & 0x0f) == 0)
				return Recover(p);

			char32_t ch3 = p[2];
			if ((ch3 & 0xc0) != 0x80)
				return Recover(p);

			ch = ((ch & 0x0f) << 12) | ((ch2 & 0x3f) << 6) | (ch3 & 0x3F);

			if ((ch & 0xD800) == 0xD800)
				return Recover(p);

			p+=3;
			return ch;
		}

		if ((ch & 0xF8) == 0xF0)
		{
			// 4-byte character

			char32_t ch2 = p[1];
			if ((ch2 & 0xc0) != 0x80 && (ch & 0x07) == 0)
				return Recover(p);

			char32_t ch3 = p[2];
			if ((ch3 & 0xc0) != 0x80)
				return Recover(p);

			char32_t ch4 = p[3];
			if ((ch3 & 0xc0) != 0x80)
				return Recover(p);

			ch = ((ch & 0x07) << 18) | ((ch2 & 0x3f) << 12) | 
					((ch3 & 0x3F) << 6) | (ch4 & 0x3f);

			if (ch >= 0x110000)
				return Recover(p);

			p+=4;
			return ch;
		}

		return Recover(p);
	}

	// Write a utf32 character to a utf8 stream
	static bool Encode(char*& p, char32_t ch)
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
};

template <typename TChar16>
struct CEncodingUtf16
{
	// Read a utf32 character from a utf16 stream
	static char32_t Decode(const TChar16*& p)
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
	static bool Encode(TChar16*& p, char32_t ch)
	{
		if (ch < 0x10000)
		{
			if ((ch & 0xD800) == 0xD800)
				return false;

			*p++ = (TChar16)ch;
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

template <typename T, int size>
struct CEncodingSelector {};

template <typename T>
struct CEncodingSelector<T, 1> : public CEncodingUtf8 {};

template <typename T>
struct CEncodingSelector<T, 2> : public CEncodingUtf16<T> {};

template <typename T>
struct CEncoding : public CEncodingSelector<T, sizeof(T)> {};


// Convert via utf32
template <typename TTo, typename TFrom>
struct CConvertViaUtf32
{
	static CCoreString<TTo> Convert(const TFrom* p)
	{
		// Start by encoding to local buffer
		TTo szBuf[1024];
		TTo* out = szBuf;
		while (*p && out < szBuf + sizeof(szBuf) - 5)
		{
			CEncoding<TTo>::Encode(out, CEncoding<TFrom>::Decode(p));
		}

		// All converted
		if (*p == 0)
		{
			*out++ = '\0';
			return szBuf;
		}

		// Need a string builder
		CCoreStringBuilder<TTo> sb;
		while (*p)
		{
			if (out >= szBuf + sizeof(szBuf) - 5)
			{
				*out = '\0';
				sb.Append(szBuf);
				out = szBuf;
			}
			CEncoding<TTo>::Encode(out, CEncoding<TFrom>::Decode(p));
		}

		// Final part
		*out = '\0';
		sb.Append(szBuf);

		return sb.ToString();
	}
};



// Conversion directly to utf32
template <typename TTo, typename TFrom>
struct CConvertToUtf32
{
	// Convert anything to utf32
	static CCoreString<TTo> Convert(const TFrom* p)
	{
		CCoreStringBuilder<TTo> sb;
		while (*p)
		{
			TTo ch = CEncoding<TFrom>::Decode(p);
			if (ch == (TTo)-1)
				ch = 0xFFFD;
			sb.Write(ch);
		}
		return sb.ToString();
	}
};

// Conversion directly from utf32
template <typename TTo, typename TFrom>
struct CConvertFromUtf32
{
	// Convert anything from utf32
	static CCoreString<TTo> Convert(const TFrom* p)
	{
		// Start by encoding to local buffer
		TTo szBuf[1024];
		TTo* out = szBuf;
		while (*p && out < szBuf + sizeof(szBuf) - 5)
		{
			CEncoding<TTo>::Encode(out, *p++);
		}

		// All converted
		if (*p == 0)
		{
			*out = '\0';
			return szBuf;
		}

		// Need a string builder
		CCoreStringBuilder<TTo> sb;
		while (*p)
		{
			if (out >= szBuf + sizeof(szBuf) - 5)
			{
				*out = '\0';
				sb.Append(szBuf);
				out = szBuf;
			}
			CEncoding<TTo>::Encode(out, *p++);
		}

		// Final part
		*out = '\0';
		sb.Append(szBuf);

		return sb.ToString();
	}
};

// Passthrough conversion
template <typename TTo, typename TFrom>
struct CConvertPassthrough
{
	// Convert anything from utf32
	static CCoreString<TTo> Convert(const TFrom* p)
	{
		return (TTo*)p;
	}
};


// Default conversion goes via utf32
template <typename TTo, typename TFrom, int sizeTo, int sizeFrom>
struct CConvertSelector : public CConvertViaUtf32<TTo, TFrom> {};

// Direct when converting from utf32
template <typename TTo, typename TFrom, int sizeTo>
struct CConvertSelector<TTo, TFrom, sizeTo, 4> : public CConvertFromUtf32<TTo, TFrom> {};

// Direct when converting to utf32
template <typename TTo, typename TFrom, int sizeFrom>
struct CConvertSelector<TTo, TFrom, 4, sizeFrom> : public CConvertToUtf32<TTo, TFrom> {};

// Passthrough conversions
template <typename TTo, typename TFrom>
struct CConvertSelector<TTo, TFrom, 1, 1> : public CConvertPassthrough<TTo, TFrom> {};
template <typename TTo, typename TFrom>
struct CConvertSelector<TTo, TFrom, 2, 2> : public CConvertPassthrough<TTo, TFrom> {};
template <typename TTo, typename TFrom>
struct CConvertSelector<TTo, TFrom, 4, 4> : public CConvertPassthrough<TTo, TFrom> {};

// Convert anything to anything class
template <typename TTo, typename TFrom>
struct CConvert : public CConvertSelector<TTo, TFrom, sizeof(TTo), sizeof(TFrom)> {};

// Function to convert anything to anything
template <typename TTo, typename TFrom>
CCoreString<TTo> Convert(const TFrom* p)
{
	return CConvert<TTo, TFrom>::Convert(p);
}

} // namespace

#endif  // __simplelib_encoding_h__