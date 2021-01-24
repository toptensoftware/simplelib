#ifndef __simplelib_formatting_h__
#define __simplelib_formatting_h__

#include <math.h>
#include <stdarg.h>

#include "stringbuilder.h"

namespace SimpleLib
{
	template <typename T>
	struct IFormatOutput
	{
		// Minimal requirement to get Format results
		virtual void Append(T ch) = 0;

		// Optional, provide optimized overrides of the following...

		virtual void Append(T ch, int count)
		{
			for (int i = 0; i < count; i++)
				Append(ch);
		}

		virtual void Append(const T* psz, int len)
		{
			for (int i = 0; i < len; i++)
			{
				Append(psz[i]);
			}
		}

		virtual void Append(const T* psz, int len, int width, bool left)
		{
			if (len < 0)
				len = 0;
			if (width > len)
			{
				if (left)
				{
					Append(psz, len);
					Append(' ', width - len);
				}
				else
				{
					Append(' ', width - len);
					Append(psz, len);
				}
			}
			else
			{
				Append(psz, len);
			}
		}
	};

	// Number formatting helpers used by CString::Format
	class CFormatting
	{
	public:

		// Supported formats:
		// %[-][+][ ][#][0][<width>][.<precision>][l|ll]<type>
		// [-] = left align
		// [+] = include positive sign
		// [ ] = display positive sign as a space
		// [#] = include '0x' (or '0X') on hex numbers and pointers, '0' on octal numbers
		// [0] = pad with leading zeros (if right aligned)
		// [l] = long
		// [ll] = long long
		// <width> = width as integer or '*' to read from arg list
		// <precision> = precision as integer or '*' to read from arg list
		// <type> = 'c', 's', 'i', 'd', 'u', 'x', 'X', 'o', 'p', 'f'
		// Note: one of the goals of this method is to provide a sprint style formatter
		//       that works exactly the same across platforms.  Old versions of CString
		//       called the crt sprintf family of functions but differences like %s %S
		//       and so on meant the format string had to be patched or the client had
		//       to provide different format strings for different platforms which is a
		//       total pain.  This might not be the perfect formatter, but at least it's
		//       consistent.
		template <class T>
		static void FormatV(IFormatOutput<T>* output, const T* format, va_list args)
		{
			// Temp buffer for formatting numbers into
			T szTemp[128];

			// Format string
			const T* p = format;
			while (*p != '\0')
			{
				if (*p == '%')
				{
					p++;

					// escaped '%' with %%?
					if (*p == '%')
					{
						output->Append('%');
						p++;
						continue;
					}

					// Parse flags
					bool bTypePrefix = false;
					bool bLeft = false;
					T chPositivePrefix = '\0';
					bool bZeroPrefix = false;
					bool bLong = false;
					bool bLongLong = false;
					bool bSizeT = false;

				next_flag:						// sometimes a goto is a friend
					if (*p == '#')
					{
						bTypePrefix = true;
						p++;
						goto next_flag;
					}

					if (*p == '-')
					{
						bLeft = true;
						p++;
						goto next_flag;
					}

					if (*p == '+')
					{
						chPositivePrefix = '+';
						p++;
						goto next_flag;
					}
					if (*p == ' ')
					{
						if (chPositivePrefix == '\0')
							chPositivePrefix = ' ';
						p++;
						goto next_flag;
					}

					// Zero prefix?
					if (*p == '0')
					{
						bZeroPrefix = !bLeft;
						p++;
					}

					// Parse width
					int iWidth = 0;
					if (*p == '*')
					{
						iWidth = va_arg(args, int);
						p++;
					}
					else
					{
						while ('0' <= *p && *p <= '9')
						{
							iWidth = iWidth * 10 + (*p++ - '0');
						}
					}

					// Parse precision
					int iPrecision = -1;
					if (*p == '.')
					{
						p++;
						bZeroPrefix = false;

						if (*p == '*')
						{
							iPrecision = va_arg(args, int);
							p++;
						}
						else
						{
							iPrecision = 0;
							while ('0' <= *p && *p <= '9')
							{
								iPrecision = iPrecision * 10 + (*p++ - '0');
							}
						}
					}

					// Type modifiers 'l' and 'll'
					if (*p == 'l')
					{
						if (p[1] == 'l')
						{
							bLongLong = true;
							p++;
						}
						else
						{
							bLong = true;
						}

						p++;
					}

					if (*p == 'z')
					{
						bSizeT = true;
						p++;
					}

					// Type specifier
					switch (*p)
					{
					case 'c':
					{
						T chArg = (char)va_arg(args, int);
						output->Append(&chArg, 1, iWidth, bLeft);
						p++;
						break;
					}

					case 's':
					{
						const T* pArg = va_arg(args, const T*);
						T temp[10];
						if (pArg == nullptr)
						{
							T* d = temp;
							for (const char* p = "(null)"; *p; d++, p++)
							{
								*d = *p;
							}
							*d++ = '\0';
							pArg = temp;
						}
						int nLen = SChar<T>::Length(pArg);
						if (nLen > iPrecision && iPrecision > 0)
							nLen = iPrecision;
						output->Append(pArg, nLen, iWidth, bLeft);
						p++;
						break;
					}

					case 'd':
					case 'i':
						if (bZeroPrefix)
						{
							iPrecision = iWidth;
							iWidth = 0;
						}

						// Process by type...
						if (bSizeT)
						{
							ptrdiff_t arg = va_arg(args, ptrdiff_t);
							if (bZeroPrefix && (arg < 0 || chPositivePrefix))
								iPrecision--;
							output->Append(szTemp, FormatSigned<T, ptrdiff_t>(szTemp, arg, iPrecision, chPositivePrefix), iWidth, bLeft);
						}
						else if (bLongLong)
						{
							long long arg = va_arg(args, long long);
							if (bZeroPrefix && (arg < 0 || chPositivePrefix))
								iPrecision--;
							output->Append(szTemp, FormatSigned<T, long long>(szTemp, arg, iPrecision, chPositivePrefix), iWidth, bLeft);
						}
						else if (bLong)
						{
							long arg = va_arg(args, long);
							if (bZeroPrefix && (arg < 0 || chPositivePrefix))
								iPrecision--;
							output->Append(szTemp, FormatSigned<T, long>(szTemp, arg, iPrecision, chPositivePrefix), iWidth, bLeft);
						}
						else
						{
							int arg = va_arg(args, int);
							if (bZeroPrefix && (arg < 0 || chPositivePrefix))
								iPrecision--;
							output->Append(szTemp, FormatSigned<T, int>(szTemp, arg, iPrecision, chPositivePrefix), iWidth, bLeft);
						}
						p++;
						break;

					case 'u':
					case 'x':
					case 'X':
					case 'o':
					{
						if (bZeroPrefix)
						{
							iPrecision = iWidth;
							iWidth = 0;
						}

						// Work out base
						int base = 10;
						if (*p == 'o')
							base = 8;
						else if (*p == 'x' || *p == 'X')
							base = 16;

						// Update case version?
						bool upper = *p == 'X';

						// Process by type...
						if (bSizeT)
						{
							size_t arg = va_arg(args, size_t);
							output->Append(szTemp, FormatUnsigned<T, size_t>(szTemp, arg, base, upper, iPrecision, bTypePrefix), iWidth, bLeft);
						}
						else if (bLongLong)
						{
							long long arg = va_arg(args, unsigned long long);
							output->Append(szTemp, FormatUnsigned<T, unsigned long long>(szTemp, arg, base, upper, iPrecision, bTypePrefix), iWidth, bLeft);
						}
						else if (bLong)
						{
							long arg = va_arg(args, unsigned long);
							output->Append(szTemp, FormatUnsigned<T, unsigned long>(szTemp, arg, base, upper, iPrecision, bTypePrefix), iWidth, bLeft);
						}
						else
						{
							int arg = va_arg(args, unsigned int);
							output->Append(szTemp, FormatUnsigned<T, unsigned int>(szTemp, arg, base, upper, iPrecision, bTypePrefix), iWidth, bLeft);
						}
						p++;
						break;
					}

					case 'p':
					case 'P':
					{
						// Pointer
						int base = 16;
						bool upper = *p == 'P';
						size_t arg = va_arg(args, size_t);
						output->Append(szTemp, FormatUnsigned<T, size_t>(szTemp, arg, base, upper, sizeof(size_t) * 2, bTypePrefix), iWidth, bLeft);
						p++;
						break;
					}

					case 'f':
					{
						// Floating point
						double val = va_arg(args, double);
						output->Append(szTemp, FormatDouble<T>(szTemp, val, iPrecision < 0 ? 6 : iPrecision, chPositivePrefix), iWidth, bLeft);
						p++;
						break;
					}

					default:
						// Huh? what was that?
						p++;
						break;
					}
				}
				else
				{
					output->Append(*p++);
				}
			}
		}


		// Helper to reverse a string (used by integer formatting)
		template <typename T>
		static int ReverseChars(T* pszStart, T* pszEnd)
		{
			// Reverse it
			T* start = pszStart;
			T* end = pszEnd - 1;
			while (end > start)
			{
				// swap
				T temp = *start;
				*start = *end;
				*end = temp;

				start++;
				end--;
			}

			// Return the length
			return (int)(pszEnd - pszStart);
		}

		// Format a signed integer
		template <typename T, typename TInt>
		static int FormatSigned(T* buf, TInt value, int padWidth, T positiveSign)
		{
			// Handle negative
			bool negative = value < 0;
			if (negative)
				value = -value;

			// Format tnumber
			T* p = buf;
			do
			{
				*p++ = (value % 10) + '0';
				value /= 10;
			} while (value != 0);

			while (p - buf < padWidth)
			{
				*p++ = '0';
			}

			// Put back the negative
			if (negative)
			{
				*p++ = '-';
			}
			else if (positiveSign)
			{
				*p++ = positiveSign;
			}
			*p = '\0';

			return ReverseChars(buf, p);
		}

		// Format an unsigned integer
		template <typename T, typename TInt>
		static int FormatUnsigned(T* buf, TInt value, int base, bool uppercase, int padWidth, bool prefix)
		{
			// Format tnumber
			T* p = buf;
			do
			{
				char ch = (char)(value % base) + '0';
				if (ch > '9')
					ch += (uppercase ? 'A' : 'a') - '9' - 1;
				*p++ = ch;
				value /= base;
			} while (value != 0);

			while (p - buf < padWidth)
			{
				*p++ = '0';
			}

			if (prefix)
			{
				if (base == 8)
					*p++ = '0';
				else if (base == 16)
				{
					*p++ = uppercase ? 'X' : 'x';
					*p++ = '0';
				}
			}

			*p = '\0';

			return ReverseChars(buf, p);
		}

		// This floating point number formatter isn't perfect, but it's pretty good.
		template <typename T>
		static int FormatDouble(T* buf, double value, int precision, T positiveSign)
		{
			T* p = buf;
			if (isnan(value))
			{
				*p++ = 'N';
				*p++ = 'a';
				*p++ = 'N';
			}
			else if (isinf(value)) 
			{
				if (value < 0)
					*p++ = '-';
				*p++ = 'i';
				*p++ = 'n';
				*p++ = 'f';
			}
			else
			{
				// check precision bounds
				if (precision < 0)
					precision = 0;

				// Handle negative
				bool negative = value < 0;
				if (negative)
					value = -value;

				// Get the integer part
				long long intPart = (long long)value;
				int roundUp = 0;

				// Get fractional part and round it according to the requested precision
				double frac = value - intPart;
				frac += 0.5 / pow(10, precision);
				if (frac >= 1.0)
				{
					roundUp = 1;
					frac -= 1.0;
				}

				// Format the integer part
				intPart += roundUp;
				do
				{
					*p++ = (intPart % 10) + '0';
					intPart /= 10;
				} while (intPart != 0);
				if (negative)
				{
					*p++ = '-';
				}
				else if (positiveSign)
				{
					*p++ = positiveSign;
				}
				ReverseChars(buf, p);

				// Format the decimal part first
				if (precision)
				{
					// place decimal point
					*p++ = '.';

					// convert
					while (precision--)
					{
						// Get the digit
						frac *= 10.0;
						int c = (int)frac;

						// Keep the fraction part
						frac -= c;

						// On the last digit, see if what's left should round up
						// This is a bit of hack but solved formatting numbers like
						// 0.12345 at 4 decimals.  Above we round to 0.1235, but on
						// after multiply by 10, we get 1.234999...
						if (precision == 0 && frac >= 0.5 && c < 9)
							c++;

						// Store the digit
						*p++ = '0' + c;
					}
				}
			}

			// Terminate and return length
			*p = '\0';
			return (int)(p - buf);
		}
	};

	// Helper class for Format functions
	template <typename T>
	class CFormatBuilder : 
		public CStringBuilder<T>,
		public IFormatOutput<T>
	{
	public:
		CFormatBuilder()
		{
		}


		void Format(const T* format, ...)
		{
			va_list args;
			va_start(args, format);
			FormatV(format, args);
			va_end(args);
		}

		void FormatV(const T* format, va_list args)
		{
			CFormatting::FormatV<T>(this, format, args);
		}

		virtual void Append(T ch) override
		{
			CStringBuilder<T>::Append(ch);
		}

		virtual void Append(T ch, int count) override
		{
			T* psz = CStringBuilder<T>::Reserve(count);
			while (count)
			{
				*psz++ = ch;
				count--;
			}
		}

		virtual void Append(const T* psz, int len) override
		{
			if (len < 0)
				len = 0;
			memcpy(CStringBuilder<T>::Reserve(len), psz, len * sizeof(T));
		}

		virtual void Append(const T* psz, int len, int width, bool left) override
		{
			if (len < 0)
				len = 0;
			if (width > len)
			{
				if (left)
				{
					Append(psz, len);
					Append(' ', width - len);
				}
				else
				{
					Append(' ', width - len);
					Append(psz, len);
				}
			}
			else
			{
				Append(psz, len);
			}
		}
	};

}	// namespace

#endif		// __simplelib_formatting_h__