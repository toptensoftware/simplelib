#pragma once

#include <stdint.h>

namespace SimpleLib
{

// Simple text parsing utilities.
// These are intended for simple parsing requirements and
// don't follow unicode chraracter classes etc...
template <typename T>
class Parse
{
public:

    // Check if a character is white space
    static bool IsWhiteSpace(T ch)
    {
        return ch=='\r' || ch=='\n' || ch==' ' || ch=='\t';
    }

    // Skip white space, return true if anything skipped
    static bool SkipWhiteSpace(const T*& p)
    {
        if (!IsWhiteSpace(p[0]))
            return false;

        p++;
        while (IsWhiteSpace(p[0]))
            p++;

        return true;
    }

    // Check if end of line character
    static bool IsEOL(T ch)
    {
        return ch=='\r' || ch=='\n';
    }

    // Skip end of line
    static bool SkipEOL(const T*& p)
    {
        if (p[0] == '\r' && p[1] == '\n')
        {
            p += 2;
            return true;
        }

        if (p[0] == '\n' || p[0] == '\r')
        {
            p++;
            return true;
        }

        return false;
    }

    // Skip to the end of the line
    static void SkipToEOL(const T*& p)
    {
        while (p[0] && !IsEOL(p[0]))
            p++;
    }

    // Skip to the start of the next line
    static void SkipToNextLine(const T*& p)
    {
        SkipToEOL(p);
        SkipEOL(p);
    }

    // Check if character is one of many
    static bool IsOneOf(T ch, const T* chars)
    {
        if (chars == nullptr)
            return false;
        while (chars[0])
        {
            if (chars[0] == ch)
                return true;
            chars++;
        }
        return false;
    }

    // Check if character is an identifier lead character
    static bool IsIdentifierLeadChar(T ch, const T* otherChars)
    {
        if (otherChars && IsOneOf(ch, otherChars))
            return true;

        return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
    }

    // Check if character is an identifier non-lead character
    static bool IsIdentifierChar(T ch, const T* otherChars)
    {
        if (otherChars && IsOneOf(ch, otherChars))
            return true;

        return (ch >= '0' && ch <= '9') || IsIdentifierLeadChar(ch, otherChars);
    }

    static bool IsDigit(T ch)
    {
        return ch >= '0' && ch <= '9';
    }

    // Parse an identifier
    static bool ParseIdentifier(const T*& p, StringCore<T>& str, const T* otherLeadChars = nullptr, const T* otherChars = nullptr)
    {
        // Store start
        const T* start=p;

        // Skip leading character
        if (!IsIdentifierLeadChar(p[0], otherLeadChars))
            return false;
        p++;

        // Skip remaining characters
        while (true)
        {
            if (IsIdentifierChar(p[0], otherChars))
            {
                p++;
                continue;
            }

            break;
        }

        // Setup return value
        str = StringCore<T>(start, int(p - start));

        return true;
    }

    // Parse an integer
    template <typename TInt>
    static bool ParseInteger(const T*& p, TInt& val)
    {
        const T* start=p;

        // Sign?
        bool bNegative=false;
        if (p[0] == '-')
        {
            p++;
            bNegative=true;
        }
        else if (p[0] == '+')
        {
            p++;
        }

        // Leading digit
        if (!IsDigit(p[0]))
        {
            p = start;
            return false;
        }

        // Other digits
        val = 0;
        while (IsDigit(p[0]))
        {
            val = val * 10 + (p[0] - '0');
            p++;
        }

        // Apply sign
        if (bNegative)
            val = -val;

        return true;
    }


    // Parse a single hex digit
    static bool ParseHexDigit(T ch, uint8_t& val)
    {
        if (ch >= '0' && ch <= '9')
        {
            val = ch - '0';
            return true;
        }
        if (ch >= 'a' && ch <= 'f')
        {
            val = ch - 'a' + 0xA;
            return true;
        }
        if (ch >= 'A' && ch <= 'F')
        {
            val = ch - 'A' + 0xA;
            return true;
        }

        return false;
    }

    // Parse a hex integer
    template <typename TUInt>
    static bool ParseHexInteger(const T*& p, TUInt& val)
    {
        if (!p)
            return false;

        const T* start=p;

        TUInt valTemp = 0;
        uint8_t bDigit;
        while (ParseHexDigit(p[0], bDigit))
        {
            valTemp = (valTemp << 4) | bDigit;
            p++;
        }

        if (p > start)
        {
            val = valTemp;
            return true;
        }

        return false;
    }

};

}