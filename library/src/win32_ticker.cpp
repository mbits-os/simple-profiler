#include "profile/ticker.hpp"
#include <Windows.h>

namespace profile
{
    namespace time
    {
        type now()
        {
            long long _now = 0;
            QueryPerformanceCounter((LARGE_INTEGER*) &_now);
            return _now;
        }

        type second()
        {
            long long _second;
            if (!QueryPerformanceFrequency((LARGE_INTEGER*) &_second))
                return 1;
            return _second;
        }
    }
}
