#include "apple/AppleUtils.hpp"
#import <Foundation/NSBundle.h>
#import <UIKit/UIKit.h>

std::string getResourcePath()
{
    return [[NSBundle mainBundle] resourcePath].UTF8String;
}

void getRetinaScreenSize(int *w, int *h)
{
    CGSize size = [[UIScreen mainScreen] bounds].size;
    CGFloat retinaScale = [[UIScreen mainScreen] scale];
    *w = size.width * retinaScale;
    *h = size.height * retinaScale;
}
