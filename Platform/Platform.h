#pragma once

#include "../FileSystem/Types.h"
#include "../Threading/Types.h"

#if defined(_WIN32)
    #include "win.h"
#else
    #include "lin.h"
#endif