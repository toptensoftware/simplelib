#pragma once

namespace SimpleLib
{

enum class EndianKind {
    Little,
    Big
};

class Endian 
{
public:

    // Read the first byte of the multi-byte integer via element indexing
    static constexpr EndianKind GetKind() {
        // Truncate/shift logic or array tricks safely evaluated at compile time
        constexpr uint32_t probe = 0x01020304;
        return (static_cast<uint8_t>(probe) == 0x04) ? EndianKind::Little : EndianKind::Big;
    }

    static inline uint16_t ByteSwap(uint16_t val)
    {
    #if defined(__GNUC__) || defined(__clang__)
        // GCC and Clang built-in for 16-bit swap
        return __builtin_bswap16(val);
    #elif defined(_MSC_VER)
        // MSVC built-in for 16-bit swap
        return _byteswap_ushort(val);
    #else
        // Fallback for standard C
        return (val << 8) | (val >> 8);
    #endif
    }


    static inline uint32_t ByteSwap(uint32_t val) 
    {
    #if defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap32(val);
    #elif defined(_MSC_VER)
        return _byteswap_ulong(val);
    #else
        // Fallback
        return ((val & 0x000000FFu) << 24) |
            ((val & 0x0000FF00u) << 8)  |
            ((val & 0x00FF0000u) >> 8)  |
            ((val & 0xFF000000u) >> 24);
    #endif
    }


    static inline uint64_t ByteSwap(uint64_t val) 
    {
        uint32_t low  = (uint32_t)val;
        uint32_t high = (uint32_t)(val >> 32);

        // Swap the bytes of each half, and shift the low half to high, high half to low
        return ((uint64_t)ByteSwap(low) << 32) | ByteSwap(high);
    }


    static inline float ByteSwap(float flt)
    {
        union
        {
            float		f;
            uint32_t	dw;
        };

        f = flt;
        dw = ByteSwap(dw);
        return f;
    }

    static inline double ByteSwap(double value)
    {
        union
        {
            double		d;
            uint64_t	dw;
        };

        d = value;
        dw = ByteSwap(dw);
        return d;
    }

    static inline int16_t ByteSwap(int16_t value)
    {
        return (int16_t)ByteSwap((uint16_t)value);
    }

    static inline int32_t ByteSwap(int32_t value)
    {
        return (int32_t)ByteSwap((uint32_t)value);
    }

    static inline int64_t ByteSwap(int64_t value)
    {
        return (int64_t)ByteSwap((uint64_t)value);
    }


    // Generate a fourcc value from a character string such that
    // ToBig(fourcc(s)) written to disk always produces the bytes in string order,
    // regardless of platform endianness (ToBig handles the platform adaptation,
    // so this value itself is intentionally platform-independent).
    static constexpr uint32_t fourcc(const char* s)
    {
        return uint32_t(uint8_t(s[3])) |
            (uint32_t(uint8_t(s[2])) << 8) |
            (uint32_t(uint8_t(s[1])) << 16) |
            (uint32_t(uint8_t(s[0])) << 24);
    }


    // Native => Big
    template <typename T>
    static inline T ToBig(T value)
    {
        if constexpr (GetKind() == EndianKind::Big)
            return value;
        else
            return ByteSwap(value);
    }

    // Native => Little
    template <typename T>
    static inline T ToLittle(T value)
    {
        if constexpr (GetKind() == EndianKind::Little)
            return value;
        else
            return ByteSwap(value);
    }

    // Big => Native
    template <typename T>
    static inline T FromBig(T value)
    {
        if constexpr (GetKind() == EndianKind::Big)
            return value;
        else
            return ByteSwap(value);
    }

    // ceeLittle => Native
    template <typename T>
    static inline T FromLittle(T value)
    {
        if constexpr (GetKind() == EndianKind::Little)
            return value;
        else
            return ByteSwap(value);
    }


};


}