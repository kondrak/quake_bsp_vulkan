#include <Foundation/NSBundle.h>

std::string getResourcePath()
{
    return [[NSBundle mainBundle] resourcePath].UTF8String;
}
