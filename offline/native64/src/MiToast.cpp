#include "MiToast.h"

USING_NS_CC;

const float MiToast::DURATION_LONG = 10.0f;
const float MiToast::DURATION_SHORT = 2.0f;

namespace {

enum ToastChildTag { kTagLabel = 0, kTagBackground = 1 };

// The original initialised these at load time (MiToast.cpp's static initialiser); the values are Bootstrap's
// alert palette, some of them mistyped in the original (info/error text, info background) and kept that way.
const ccColor4B kDebugText = {0x3a, 0x87, 0xad, 0xff};
const ccColor4B kDebugBackground = {0xd9, 0xed, 0xf7, 0xc8};
const ccColor4B kInfoText = {0x46, 0x88, 0x04, 0xff};
const ccColor4B kInfoBackground = {0xdf, 0xf0, 0x0d, 0xc8};
const ccColor4B kWarnText = {0xc0, 0x98, 0x53, 0xc8};
const ccColor4B kWarnBackground = {0xfc, 0xf8, 0xe3, 0xc8};
const ccColor4B kErrorText = {0xb9, 0x59, 0x94, 0xff};
const ccColor4B kErrorBackground = {0xf2, 0xde, 0xde, 0xc8};
// Background layer colour until show() recolours it.
const ccColor4B kInitialBackground = {0xf3, 0xf3, 0xf3, 0xc8};

// Layout was designed for a 640-pixel-wide screen.
const float kDesignWidth = 640.0f;
const float kFontSize = 32.0f;

}  // namespace

MiToast::~MiToast() {}

MiToast* MiToast::create() {
    MiToast* toast = new MiToast();
    if (toast->init()) {
        toast->autorelease();
        return toast;
    }
    delete toast;
    return nullptr;
}

// orig 001dd868
bool MiToast::init() {
    if (!CCLayer::init()) return false;

    CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    float scale = winSize.width / kDesignWidth;

    CCLayerColor* background = CCLayerColor::create(kInitialBackground);
    background->ignoreAnchorPointForPosition(false);
    background->setAnchorPoint(ccp(0.5f, 0.5f));
    addChild(background, 100, kTagBackground);

    CCLabelTTF* label = CCLabelTTF::create("", "Arial", scale * kFontSize);
    label->setHorizontalAlignment(kCCTextAlignmentCenter);
    label->setVerticalAlignment(kCCVerticalTextAlignmentCenter);
    label->setAnchorPoint(ccp(0.5f, 0.5f));
    addChild(label, 100, kTagLabel);

    background->setContentSize(label->getContentSize());
    setContentSize(label->getContentSize());
    ignoreAnchorPointForPosition(false);
    setAnchorPoint(ccp(0.5f, 0.5f));
    setPosition(ccp(winSize.width * 0.5f, winSize.height * 0.5f));

    // Deviation: the original added itself to the running scene unconditionally (null dereference without one).
    if (CCScene* scene = CCDirector::sharedDirector()->getRunningScene()) scene->addChild(this);
    return true;
}

// orig 001dd6bc
void MiToast::show(const std::string& text, float duration, ccColor4B textColor, ccColor4B backgroundColor) {
    MiToast* toast = create();
    // No running scene: nothing to show it on (and a paused schedule would keep the toast alive forever).
    if (!toast || !toast->getParent()) return;

    CCLabelTTF* label = static_cast<CCLabelTTF*>(toast->getChildByTag(kTagLabel));
    label->setString(text.c_str());
    label->setColor(ccc3(textColor.r, textColor.g, textColor.b));

    CCLayerColor* background = static_cast<CCLayerColor*>(toast->getChildByTag(kTagBackground));
    // Kept from the original: the red channel is taken from the green component.
    background->setColor(ccc3(backgroundColor.g, backgroundColor.g, backgroundColor.b));
    background->setOpacity(backgroundColor.a);
    background->setContentSize(label->getContentSize());

    // The original scheduled dismiss() itself, cast to a schedule selector.
    toast->schedule(schedule_selector(MiToast::onDurationElapsed), duration);
}

void MiToast::debug(const std::string& text, float duration) {
    show(text, duration, kDebugText, kDebugBackground);
}

void MiToast::info(const std::string& text, float duration) {
    show(text, duration, kInfoText, kInfoBackground);
}

void MiToast::warn(const std::string& text, float duration) {
    show(text, duration, kWarnText, kWarnBackground);
}

void MiToast::error(const std::string& text, float duration) {
    show(text, duration, kErrorText, kErrorBackground);
}

void MiToast::dismiss() {
    removeFromParentAndCleanup(true);
}

void MiToast::onDurationElapsed(float) {
    dismiss();
}
