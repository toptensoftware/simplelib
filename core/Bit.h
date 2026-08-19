#pragma once

namespace SimpleLib
{

template <typename T>
struct is_unsigned_bitmask
{
    static constexpr bool value = T(-1) > T(0);
};

class Bit
{
public:
    // Count set bits
    template <typename T>
    static int Count(T n)
    {
        static_assert(is_unsigned_bitmask<T>::value, "requires unsigned T");
        int iCount = 0;
        while (n)
        {
            if (n & (T)1)
                iCount++;
            n >>= 1;
        }
        return iCount;
    }

    // Set a bit
    template <typename T>
    static void Set(T& n, int iBit)
    {
        static_assert(is_unsigned_bitmask<T>::value, "requires unsigned T");
        n |= (((T)1) << iBit); 
    }

    // Clear a bit
    template <typename T>
    static void Clear(T& n, int iBit)
    {
        static_assert(is_unsigned_bitmask<T>::value, "requires unsigned T");
        n &= ~(((T)1) << iBit);
    }

    // Set (or clear a bit)
    template <typename T>
    static void Set(T& n, int iBit, bool set)
    {
        if (set)
            Set(n, iBit);
        else
            Clear(n, iBit);
    }

    // Find the first set bit
    template <typename T>
    static int Scan(T mask)
    {
        return Find(mask, 0);
    }

    // Find the index'th set bit
    template <typename T>
    static int Find(T mask, int index)
    {
        static_assert(is_unsigned_bitmask<T>::value, "requires unsigned T");

        int bit = 0;
        while (mask)
        {
            if (mask & 1)
            {
                if (index == 0)
                    return bit;
                index--;
            }
            mask >>= 1;
            bit++;
        }
        return -1;
    }
};

}