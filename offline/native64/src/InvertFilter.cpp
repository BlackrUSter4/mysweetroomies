#include "InvertFilter.h"

USING_NS_CC;

const std::string InvertFilter::VERTEX_SHADER = "Framework/shader/Common.vsh";
const std::string InvertFilter::FRAGMENT_SHADER = "Framework/shader/Invert.fsh";

// orig 001dde58
InvertFilter::~InvertFilter() {}

// orig 001d65ec
BaseFilter *InvertFilter::create(const std::string &imagePath)
{
    InvertFilter *sprite = new InvertFilter();
    if (!sprite->initWithFile(imagePath.c_str())) {
        delete sprite;
        return nullptr;
    }
    sprite->autorelease();
    return sprite;
}

// orig 001d6660
BaseFilter *InvertFilter::createWithSpriteFrame(CCSpriteFrame *frame)
{
    InvertFilter *sprite = new InvertFilter();
    if (frame == nullptr || !sprite->initWithSpriteFrame(frame)) {
        delete sprite;
        return nullptr;
    }
    sprite->autorelease();
    return sprite;
}

// orig 001dde6c
void InvertFilter::prepare()
{
    BaseFilter::prepare(VERTEX_SHADER, FRAGMENT_SHADER);
}
