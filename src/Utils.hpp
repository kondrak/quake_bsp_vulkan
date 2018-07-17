#ifndef UTILS_HPP
#define UTILS_HPP
#include <sstream>

#ifdef _DEBUG
#define LOG_MESSAGE(msg) { \
    std::stringstream msgStr; \
    msgStr << "[LOG]: " << msg << "\n"; \
    LogError(msgStr.str().c_str()); \
}

#define LOG_MESSAGE_ASSERT(x, msg) \
    if(!(x)) { \
    std::stringstream msgStr; \
    msgStr << "[!ASSERT!]: " << msg << "\n"; \
    LogError(msgStr.str().c_str()); \
    Break(); \
    }
#else
#define LOG_MESSAGE(msg)
#define LOG_MESSAGE_ASSERT(x, msg)
#endif


void LogError(const char *msg);
void Break();
#endif
