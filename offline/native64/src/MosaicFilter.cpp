#include "MosaicFilter.h"

USING_NS_CC;

const std::string MosaicFilter::VERTEX_SHADER = "Framework/shader/Common.vsh";
const std::string MosaicFilter::FRAGMENT_SHADER = "Framework/shader/Mosaic.fsh";

// orig 001ddebc
MosaicFilter::~MosaicFilter() {}

// orig 001d67c4
BaseFilter *MosaicFilter::create(const std::string &imagePath)
{
    MosaicFilter *sprite = new MosaicFilter();
    if (!sprite->initWithFile(imagePath.c_str())) {
        delete sprite;
        return nullptr;
    }
    sprite->autorelease();
    return sprite;
}

// orig 001d6838
BaseFilter *MosaicFilter::createWithSpriteFrame(CCSpriteFrame *frame)
{
    MosaicFilter *sprite = new MosaicFilter();
    if (frame == nullptr || !sprite->initWithSpriteFrame(frame)) {
        delete sprite;
        return nullptr;
    }
    sprite->autorelease();
    return sprite;
}

// orig 001dded0
void MosaicFilter::prepare()
{
    BaseFilter::prepare(VERTEX_SHADER, FRAGMENT_SHADER);
}
