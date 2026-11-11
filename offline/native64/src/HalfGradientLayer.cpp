#include "HalfGradientLayer.h"

USING_NS_CC;

namespace {
const int kSolidHalfTag = 0;
const int kGradientHalfTag = 1;
}  // namespace

// orig 001de564
HalfGradientLayer *HalfGradientLayer::create()
{
    HalfGradientLayer *layer = new HalfGradientLayer();
    if (!layer->init()) {
        delete layer;
        return nullptr;
    }
    layer->autorelease();
    return layer;
}

// orig 001e0114
HalfGradientLayer::~HalfGradientLayer() {}

// orig 001e0256
bool HalfGradientLayer::init()
{
    if (!CCLayer::init()) return false;

    // Everything is measured in screen widths: a width x width square, split horizontally.
    const float halfWidth = CCDirector::sharedDirector()->getWinSize().width * 0.5f;
    const float width = halfWidth + halfWidth;

    CCLayerColor *solid = CCLayerColor::create(ccc4(0, 0, 0, 255));
    solid->setContentSize(CCSize(width, halfWidth));
    solid->setPosition(CCPoint(0.0f, halfWidth));
    addChild(solid, kSolidHalfTag, kSolidHalfTag);

    CCLayerGradient *gradient = CCLayerGradient::create(ccc4(0, 0, 0, 255), ccc4(0, 0, 0, 0));
    gradient->setContentSize(CCSize(width, halfWidth));
    gradient->setPosition(CCPoint(0.0f, 0.0f));
    addChild(gradient, kGradientHalfTag, kGradientHalfTag);

    setContentSize(CCSize(width, width));
    return true;
}

// orig 001e018c
void HalfGradientLayer::setLayerColor(ccColor4B color)
{
    const ccColor3B rgb = ccc3(color.r, color.g, color.b);

    CCLayerColor *solid = static_cast<CCLayerColor *>(getChildByTag(kSolidHalfTag));
    solid->setColor(rgb);
    solid->setOpacity(color.a);

    CCLayerGradient *gradient = static_cast<CCLayerGradient *>(getChildByTag(kGradientHalfTag));
    gradient->setStartColor(rgb);
    gradient->setStartOpacity(color.a);
    gradient->setEndColor(rgb);
    gradient->setEndOpacity(0);
}
