#include "apple/AppleUtils.hpp"
#import <Foundation/NSBundle.h>
#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#endif

std::string getResourcePath()
{
    return [[NSBundle mainBundle] resourcePath].UTF8String;
}

void getRetinaScreenSize(int *w, int *h)
{
#if TARGET_OS_IPHONE
    CGSize size = [[UIScreen mainScreen] bounds].size;
    CGFloat retinaScale = [[UIScreen mainScreen] scale];
    *w = size.width * retinaScale;
    *h = size.height * retinaScale;
#endif
}
