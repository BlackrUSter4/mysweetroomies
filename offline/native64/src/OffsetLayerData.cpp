#include "OffsetLayerData.h"

USING_NS_CC;

namespace {
// Width/height of the square the offset file's coordinates are normalised against (the original's literal 480.0f).
const float kOffsetReferenceSize = 480.0f;
}  // namespace

OffsetLayerData::OffsetLayerData(const char *imagePath, float offsetX, float offsetY) : x(offsetX), y(offsetY)
{
    path = imagePath;
}

OffsetLayerData::~OffsetLayerData() {}

CCPoint OffsetLayerData::getOffsetPoint(CCSize imageSize)
{
    CCPoint anchor((imageSize.width * 0.5f + x) / kOffsetReferenceSize,
                   1.0f - (imageSize.height * 0.5f + y) / kOffsetReferenceSize);
    return CCPoint(anchor.x - 0.5f, anchor.y - 0.5f);
}

CCPoint OffsetLayerData::getOffsetPoint()
{
    CCSprite *sprite = CCSprite::create(path.c_str());
    return getOffsetPoint(sprite->getContentSize());
}
