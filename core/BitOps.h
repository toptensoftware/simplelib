#pragma once

namespace SimpleLib
{

template <typename T>
struct is_unsigned_bitmask
{
    static constexpr bool value = T(-1) > T(0);
};


template <typename T>
int bitCount(T n)
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

template <typename T>
void bitSet(T& n, int iBit)
{
    static_assert(is_unsigned_bitmask<T>::value, "requires unsigned T");
    n |= (((T)1) << iBit); 
}

template <typename T>
void bitClear(T& n, int iBit)
{
    static_assert(is_unsigned_bitmask<T>::value, "requires unsigned T");
    n &= ~(((T)1) << iBit);
}

template <typename T>
void bitSet(T& n, int iBit, bool set)
{
    if (set)
        bitSet(n, iBit);
    else
        bitClear(n, iBit);
}

template <typename T>
int FindNthSetBit(T mask, int n)
{
    static_assert(is_unsigned_bitmask<T>::value, "requires unsigned T");

    int bit = 0;
    while (mask)
    {
        if (mask & 1)
        {
            if (n == 0)
                return bit;
            n--;
        }
        mask >>= 1;
        bit++;
    }
    return -1;
}

}