#pragma once

namespace SimpleLib
{

#include <stdint.h>
#include <stddef.h>

// FNV-1a 32-bit hash
// seed lets a hash be continued/folded across multiple calls (e.g. hashing
// a string one case-folded character at a time without a temporary buffer)
inline uint32_t hash_buf(const void* data, size_t len, uint32_t seed = 0x811c9dc5u)
{
    const uint8_t* p = (const uint8_t*)data;
    uint32_t h = seed;      // FNV offset basis by default
    for (size_t i = 0; i < len; i++)
    {
        h ^= p[i];
        h *= 0x01000193u;          // FNV prime
    }
    return h;
}

// Convenience wrapper for null-terminated strings
inline uint32_t hash_str(const char* s)
{
    uint32_t h = 0x811c9dc5u;
    while (*s)
    {
        h ^= (uint8_t)*s++;
        h *= 0x01000193u;
    }
    return h;
}


// 32-bit integer hash
// https://stackoverflow.com/a/12996028/77002
inline uint32_t hash32(uint32_t x) {
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = (x >> 16) ^ x;
	return x;
}

inline uint64_t ror64(uint64_t v, int r) {
	return (v >> r) | (v << (64 - r));
}

// 64-bit hash
// https://stackoverflow.com/a/56501471/77002
inline uint64_t hash64(uint64_t v) 
{
	v ^= ror64(v, 25) ^ ror64(v, 50);
	v *= 0xA24BAED4963EE407UL;
	v ^= ror64(v, 24) ^ ror64(v, 49);
	v *= 0x9FB21C651E98DF25UL;
	return v ^ v >> 28;
}

// 64-bit hash to 32-bit value
inline uint32_t hash32_64(uint64_t v)
{
	uint64_t hash = hash64(v);
	return (uint32_t)hash ^ (uint32_t)(hash >> 32);
}


// Check if a number is prime
inline bool is_prime(int32_t val)
{
	if ((val & 1) != 0)
	{
		for (int32_t divisor = 3; divisor * divisor <= val; divisor += 2)
		{
			if ((val % divisor) == 0)
				return false;
		}
		return true;
	}
	return (val == 2);
}

inline int32_t next_prime_capacity(int32_t min)
{
	// list of early primes (not all)
	static int32_t primes[] = {
		3, 7, 11, 17, 23, 29, 37, 47, 59, 71, 89, 107, 131, 163, 197, 239, 293, 353, 431, 521, 631, 761, 919,
		1103, 1327, 1597, 1931, 2333, 2801, 3371, 4049, 4861, 5839, 7013, 8419, 10103, 12143, 14591,
		17519, 21023, 25229, 30293, 36353, 43627, 52361, 62851, 75431, 90523, 108631, 130363, 156437,
		187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403, 968897, 1162687, 1395263,
		1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 4999559, 5999471, 7199369,

	};

	// max 64-bit prime
	static int32_t max_prime = 2147483647;

	// Find the next in our table
	int count = sizeof(primes) / sizeof(int);
	for (int i = 0; i < count; i++)
	{
		if (primes[i] >= min)
			return primes[i];
	}

	// Slow method
	min |= 1;
	while (min < max_prime && !is_prime(min))
		min += 2;

	return min;
}



}