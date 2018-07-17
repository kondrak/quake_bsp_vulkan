#include "Utils.hpp"
#ifdef _WIN32
#include <Windows.h>
#endif

void LogError(const char *msg)
{
    // basic error logging for VS debugger
#ifdef _WIN32
    OutputDebugStringA(msg);
#else
    printf("%s\n", msg);
#endif
}

void Break()
{
#ifdef _WIN32
    __debugbreak();
#else
    abort();
#endif
}
