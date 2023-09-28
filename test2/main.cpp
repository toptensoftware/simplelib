#include <stdio.h>
#include "../SimpleLib.h"
using namespace SimpleLib;

int main()
{
    auto str = String<char>::Format("Test %i\n", 23);
    printf(str.sz());
    return 0;
}