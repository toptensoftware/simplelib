#pragma once

#include "../FileSystem/Types.h"
#include "../Threading/Types.h"

#if defined(_WIN32)
    #include "Win.h"
#else
    #include "Lin.h"
#endif