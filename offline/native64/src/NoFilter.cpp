#include "NoFilter.h"

USING_NS_CC;

// orig 001ddef0
NoFilter::~NoFilter() {}

// orig 001d68b0
BaseFilter *NoFilter::create(const std::string &imagePath)
{
    NoFilter *sprite = new NoFilter();
    if (!sprite->initWithFile(imagePath.c_str())) {
        delete sprite;
        return nullptr;
    }
    sprite->autorelease();
    return sprite;
}

// orig 001ddeec
void NoFilter::prepare() {}
