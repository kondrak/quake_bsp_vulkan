#include "apple/AppleUtils.hpp"
#import <Foundation/NSBundle.h>

std::string getResourcePath()
{
    return [[[NSBundle mainBundle] resourcePath] stringByAppendingString:@"/"].UTF8String;
}
