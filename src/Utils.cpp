#include "Utils.hpp"
#ifdef _WIN32
#include <Windows.h>
#endif
#ifdef __ANDROID__
#include <android/log.h>
#endif

void LogError(const char *msg)
{
    // basic error logging for VS debugger
#if defined _WIN32
    OutputDebugStringA(msg);
#elif defined __ANDROID__
    __android_log_print(ANDROID_LOG_ERROR, "QuakeBspViewer", "%s\n", msg);
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
