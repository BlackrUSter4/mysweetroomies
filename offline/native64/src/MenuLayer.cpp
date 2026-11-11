#include "MenuLayer.h"

USING_NS_CC;

namespace {

// These two buttons were laid out for a 480-pixel-wide design, unlike the 640 of the rest of the ui.
const float kDesignWidth = 480.0f;
const float kMargin = 10.0f;  // design pixels from the screen edges

// Height of the band the corner buttons sit above, measured down from the screen centre. The original took it
// from a CCSize built out of the window *width* twice (so the buttons sit winWidth below the centre on a
// portrait screen); kept as is.
float buttonBandHeight(const CCSize& winSize)
{
    return winSize.width;
}

}  // namespace

// orig 001c861c
MenuLayer::~MenuLayer() {}

// orig 001e06e4
bool MenuLayer::init()
{
    return CCLayer::init();
}

// orig 001e056c
void MenuLayer::initUi() {}

// orig 001e056e
const char* MenuLayer::prepare(const char* plist, const char* texture)
{
    CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile(plist, texture);
    return plist;
}

// orig 001e0586
void MenuLayer::setMenu(const char* normalFrame, const char* selectedFrame, SEL_MenuHandler selector,
                        std::function<void(CCMenuItemSprite*)> layout)
{
    CCMenuItemSprite* item = CCMenuItemSprite::create(CCSprite::createWithSpriteFrameName(normalFrame),
                                                      CCSprite::createWithSpriteFrameName(selectedFrame), this, selector);
    CCMenu* menu = CCMenu::create(item, nullptr);
    layout(item);
    addChild(menu);
}

// orig 001e05e4 (layout lambda 001e0394)
void MenuLayer::setVoiceButton(const char* normalFrame, const char* selectedFrame, SEL_MenuHandler selector)
{
    setMenu(normalFrame, selectedFrame, selector, [](CCMenuItemSprite* item) {
        CCSize winSize = CCDirector::sharedDirector()->getWinSize();
        float scale = winSize.width / kDesignWidth;
        float margin = scale * kMargin;
        item->setAnchorPoint(ccp(1.0f, 0.0f));
        item->setPosition(ccp(winSize.width * 0.5f - margin, winSize.height * 0.5f - buttonBandHeight(winSize) + margin));
        item->setScale(scale);
    });
}

// orig 001e0664 (layout lambda 001e0444)
void MenuLayer::setMenuPopButton(const char* normalFrame, const char* selectedFrame, SEL_MenuHandler selector)
{
    setMenu(normalFrame, selectedFrame, selector, [](CCMenuItemSprite* item) {
        CCSize winSize = CCDirector::sharedDirector()->getWinSize();
        float scale = winSize.width / kDesignWidth;
        float margin = scale * kMargin;
        item->setAnchorPoint(ccp(0.0f, 0.0f));
        item->setPosition(ccp(-winSize.width * 0.5f + margin, winSize.height * 0.5f - buttonBandHeight(winSize) + margin));
        item->setScale(scale);
    });
}
