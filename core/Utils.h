#pragma once

namespace SimpleLib
{

class Utils
{
public:

    template <typename T>
    static void Swap(T& a, T& b)
    {
        T temp = a;
        a = b;
        b = temp;
    }
};

}