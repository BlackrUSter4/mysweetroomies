#include "SepiaFilter.h"

USING_NS_CC;

const std::string SepiaFilter::VERTEX_SHADER = "Framework/shader/Common.vsh";
const std::string SepiaFilter::FRAGMENT_SHADER = "Framework/shader/Sepia.fsh";

// orig 001ddf6c
SepiaFilter::~SepiaFilter() {}

// orig 001d6500
BaseFilter *SepiaFilter::create(const std::string &imagePath)
{
    SepiaFilter *sprite = new SepiaFilter();
    if (!sprite->initWithFile(imagePath.c_str())) {
        delete sprite;
        return nullptr;
    }
    sprite->autorelease();
    return sprite;
}

// orig 001d6574
BaseFilter *SepiaFilter::createWithSpriteFrame(CCSpriteFrame *frame)
{
    SepiaFilter *sprite = new SepiaFilter();
    if (frame == nullptr || !sprite->initWithSpriteFrame(frame)) {
        delete sprite;
        return nullptr;
    }
    sprite->autorelease();
    return sprite;
}

// orig 001ddf80
void SepiaFilter::prepare()
{
    BaseFilter::prepare(VERTEX_SHADER, FRAGMENT_SHADER);
}
