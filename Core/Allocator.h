#pragma once

#include <stdlib.h>

namespace SimpleLib
{

class TMalloc
{
public:
    static void* Alloc(size_t size)
    {
        return malloc(size);
    }
    static void* ReAlloc(void* ptr, size_t size)
    {
        return realloc(ptr, size);
    }
    static void Free(void* ptr)
    {
        free(ptr);
    }
};

}