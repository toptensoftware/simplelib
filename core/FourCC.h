#pragma once

#include "Endian.h"

namespace SimpleLib
{


// Generate a fourcc value from a character string such that the bytes
// in memory are the same as the character string
static constexpr uint32_t cc4(const char* s)
{
    if constexpr (Endian::GetKind() == EndianKind::Little)
    {
        return uint32_t(uint8_t(s[0])) |
            (uint32_t(uint8_t(s[1])) << 8) |
            (uint32_t(uint8_t(s[2])) << 16) |
            (uint32_t(uint8_t(s[3])) << 24);
    }
    else
    {
        return uint32_t(uint8_t(s[3])) |
            (uint32_t(uint8_t(s[2])) << 8) |
            (uint32_t(uint8_t(s[1])) << 16) |
            (uint32_t(uint8_t(s[0])) << 24);
    }
}


static constexpr uint64_t cc8(const char* s)
{
    if constexpr (Endian::GetKind() == EndianKind::Little)
    {
        return uint64_t(uint8_t(s[0])) |
            (uint64_t(uint8_t(s[1])) << 8) |
            (uint64_t(uint8_t(s[2])) << 16) |
            (uint64_t(uint8_t(s[3])) << 24) |
            (uint64_t(uint8_t(s[4])) << 32) |
            (uint64_t(uint8_t(s[5])) << 40) |
            (uint64_t(uint8_t(s[6])) << 48) |
            (uint64_t(uint8_t(s[7])) << 56);
    }
    else
    {
        return uint64_t(uint8_t(s[7])) |
            (uint64_t(uint8_t(s[6])) << 8) |
            (uint64_t(uint8_t(s[5])) << 16) |
            (uint64_t(uint8_t(s[4])) << 24) |
            (uint64_t(uint8_t(s[3])) << 32) |
            (uint64_t(uint8_t(s[2])) << 40) |
            (uint64_t(uint8_t(s[1])) << 48) |
            (uint64_t(uint8_t(s[0])) << 56);
    }
}



}