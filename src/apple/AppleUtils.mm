#include "apple/AppleUtils.hpp"
#import <Foundation/NSBundle.h>

std::string getResourcePath()
{
    return [[NSBundle mainBundle] resourcePath].UTF8String;
}
