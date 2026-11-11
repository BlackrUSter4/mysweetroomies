#include "GrayScaleFilter.h"

USING_NS_CC;

const std::string GrayScaleFilter::VERTEX_SHADER = "Framework/shader/Common.vsh";
const std::string GrayScaleFilter::FRAGMENT_SHADER = "Framework/shader/GrayScale.fsh";

// orig 001dddf4
GrayScaleFilter::~GrayScaleFilter() {}

// orig 001d6414
BaseFilter *GrayScaleFilter::create(const std::string &imagePath)
{
    GrayScaleFilter *sprite = new GrayScaleFilter();
    if (!sprite->initWithFile(imagePath.c_str())) {
        delete sprite;
        return nullptr;
    }
    sprite->autorelease();
    return sprite;
}

// orig 001d6488
BaseFilter *GrayScaleFilter::createWithSpriteFrame(CCSpriteFrame *frame)
{
    GrayScaleFilter *sprite = new GrayScaleFilter();
    if (frame == nullptr || !sprite->initWithSpriteFrame(frame)) {
        delete sprite;
        return nullptr;
    }
    sprite->autorelease();
    return sprite;
}

// orig 001dde08
void GrayScaleFilter::prepare()
{
    BaseFilter::prepare(VERTEX_SHADER, FRAGMENT_SHADER);
}
