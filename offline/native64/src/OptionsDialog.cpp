#include "OptionsDialog.h"

#include <cmath>

#include "CCControlSliderTouchable.h"
#include "SceneManager.h"
#include "SoundEngine.h"
#include "UiHelpers.h"

USING_NS_CC;
USING_NS_CC_EXT;

namespace {

const float kDesignWidth = 640.0f;
// Lower edge of the frame and the heights of its rows, in design pixels above that edge.
const float kFrameBottom = 200.0f;
const float kBgmRowY = 490.0f;
const float kSeRowY = 350.0f;
const float kTextSpeedRowY = 210.0f;
const float kResetRowY = 85.0f;
// Keep in step with setAutoTextValueFromSlider() (UiHelpers.cpp).
const float kMinAutoPlayDelay = 0.25f;

// Track, filled part and thumb of every slider.
const char* const kSliderTrack = "sliderTrack.png";
const char* const kSliderProgress = "sliderProgress.png";
const char* const kSliderThumb = "sliderThumb.png";

const char* const kBgmSample = "Framework/sound/bgm/bgm_sample.ogg";
const char* const kVoiceSample = "Framework/sound/voice/vo_sample.ogg";

}  // namespace

// orig 001caa58
OptionsDialog* OptionsDialog::create()
{
    OptionsDialog* dialog = new OptionsDialog();
    if (!dialog->init()) {
        delete dialog;
        return nullptr;
    }
    dialog->autorelease();
    return dialog;
}

// orig 001e08d4
CCScene* OptionsDialog::scene()
{
    CCScene* scene = CCScene::create();
    scene->addChild(create());
    return scene;
}

// orig 001e07e8
OptionsDialog::~OptionsDialog() {}

// orig 001e071c
bool OptionsDialog::init()
{
    if (!MiModalLayer::init()) return false;
    initUi();
    return true;
}

// orig 001e0ae0
void OptionsDialog::initUi()
{
    CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile("Framework/ui/menu_option.plist",
                                                                          "Framework/ui/menu_option.png");
    CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    float scale = winSize.width / kDesignWidth;
    float frameBottom = scale * kFrameBottom;

    CCSprite* frame = CCSprite::createWithSpriteFrameName("bg_setting.png");
    matchParentWidth(frame, 1.0f);
    frame->setAnchorPoint(ccp(0.5f, 0.0f));
    frame->setPosition(ccp(winSize.width * 0.5f, frameBottom));
    addChild(frame);

    buildSlider(cccontrol_selector(OptionsDialog::onBgmSliderEvent), scale * kBgmRowY + frameBottom,
                OptionsDialogTag_Bgm, 0.0f, 1.0f, SoundEngine::getInstance()->getVolumeBgm(), "bgm.png",
                [] { SoundEngine::getInstance()->playBgm(kBgmSample); });
    buildSlider(cccontrol_selector(OptionsDialog::onSeSliderEvent), scale * kSeRowY + frameBottom,
                OptionsDialogTag_Se, 0.0f, 1.0f, SoundEngine::getInstance()->getVolumeSe(), "se_vo.png",
                [] { SoundEngine::getInstance()->playVoice(kVoiceSample); });

    // The slider runs from slow (left) to fast (right): its value is 1 - the delay's position in
    // [kMinAutoPlayDelay, 2 * AUTOPLAY_DELAY - kMinAutoPlayDelay].
    float delayRange = (SceneManager::AUTOPLAY_DELAY - kMinAutoPlayDelay) * 2;
    float delayPosition = (SceneManager::getInstance()->getAutoPlayDelay() - kMinAutoPlayDelay) / delayRange;
    float textSpeed = static_cast<float>(std::fabs(1.0 - delayPosition));
    buildSlider(cccontrol_selector(OptionsDialog::onTextSpeedSliderEvent), scale * kTextSpeedRowY + frameBottom,
                OptionsDialogTag_TextSpeed, 0.0f, 1.0f, textSpeed, "text_speed.png", [] {});

    CCMenuItemSprite* resetItem = CCMenuItemSprite::create(CCSprite::createWithSpriteFrameName("btn_reset.png"),
                                                           CCSprite::createWithSpriteFrameName("btn_reset_active.png"),
                                                           this, menu_selector(OptionsDialog::onDefaultButtonClicked));
    resetItem->setScale(scale);
    CCMenu* resetMenu = CCMenu::create(resetItem, nullptr);
    resetMenu->setPosition(ccp(winSize.width * 0.5f, scale * kResetRowY + frameBottom));
    int buttonPriority = MiModalLayer::kModalLayerPriority - 2;
    resetMenu->setTouchPriority(buttonPriority);
    addChild(resetMenu, 1, OptionsDialogTag_Reset);

    // Close button hanging over the frame's top-right corner.
    CCMenuItemSprite* closeItem =
        CCMenuItemSprite::create(CCSprite::createWithSpriteFrameName("btn_window_close.png"),
                                 CCSprite::createWithSpriteFrameName("btn_window_close_active.png"), this,
                                 menu_selector(OptionsDialog::onCloseButtonClicked));
    closeItem->setScale(scale);
    closeItem->setAnchorPoint(ccp(1.3f, 1.3f));
    CCMenu* closeMenu = CCMenu::create(closeItem, nullptr);
    CCRect frameBox = frame->boundingBox();
    closeMenu->setPosition(ccp(frameBox.getMaxX(), frameBox.getMaxY()));
    closeMenu->setTouchPriority(buttonPriority);
    addChild(closeMenu);

    setEnableCloseOnTouchLayer(false);
}

// orig 001e0888
void OptionsDialog::dismiss()
{
    SoundEngine::getInstance()->stop();
    MiModalLayer::dismiss();
}

// orig 001e08f0
void OptionsDialog::buildSlider(SEL_CCControlHandler selector, float y, int tag, float min, float max, float value,
                                const char* labelFrame, std::function<void()> onTouchEnd)
{
    CCSize winSize = CCDirector::sharedDirector()->getWinSize();

    CCSprite* label = CCSprite::createWithSpriteFrameName(labelFrame);
    matchParentWidth(label, 0.9f);
    label->setPosition(ccp(winSize.width * 0.5f, y));
    addChild(label);
    float labelHeight = label->getContentSize().height * label->getScale();

    CCControlSliderTouchable* slider = CCControlSliderTouchable::create(
        CCSprite::createWithSpriteFrameName(kSliderTrack), CCSprite::createWithSpriteFrameName(kSliderProgress),
        CCSprite::createWithSpriteFrameName(kSliderThumb));
    slider->setAnchorPoint(ccp(0.5f, 0.5f));
    // The original computed this offset in double precision.
    slider->setPosition(ccp(winSize.width * 0.5f, static_cast<float>(static_cast<double>(y) - labelHeight * 0.3)));
    slider->setTouchPriority(MiModalLayer::kModalLayerPriority - 1);
    slider->setMaximumValue(max);
    slider->setMinimumValue(min);
    // Set before the target is attached, so building the dialog does not fire the handler.
    slider->setValue(value);
    slider->addTargetWithActionForControlEvents(this, selector, CCControlEventValueChanged);
    slider->setOnTouchEndListener(onTouchEnd);
    matchParentWidth(slider, 0.75f);
    addChild(slider, 1, tag);
}

// orig 001e089c
void OptionsDialog::onBgmChanged(CCControlSlider* slider)
{
    SoundEngine::getInstance()->setVolumeBgm(slider->getValue());
}

// orig 001e08b8
void OptionsDialog::onSeChanged(CCControlSlider* slider)
{
    SoundEngine::getInstance()->setVolumeSe(slider->getValue());
}

// orig 001e0f4c
void OptionsDialog::onAutoTextChanged(CCControlSlider* slider)
{
    setAutoTextValueFromSlider(std::fabs(slider->getMaximumValue() - slider->getValue()));
}

// orig 001e0fec
void OptionsDialog::onDefaultButtonClicked(CCObject* /*sender*/)
{
    setDefaultValue(this, OptionsDialogTag_Bgm);
    setDefaultValue(this, OptionsDialogTag_Se);
    setDefaultValue(this, OptionsDialogTag_TextSpeed);
}

// orig 001e06ec
void OptionsDialog::onCloseButtonClicked(CCObject* /*sender*/)
{
    dismiss();
}

void OptionsDialog::onBgmSliderEvent(CCObject* sender, CCControlEvent /*event*/)
{
    onBgmChanged(static_cast<CCControlSlider*>(sender));
}

void OptionsDialog::onSeSliderEvent(CCObject* sender, CCControlEvent /*event*/)
{
    onSeChanged(static_cast<CCControlSlider*>(sender));
}

void OptionsDialog::onTextSpeedSliderEvent(CCObject* sender, CCControlEvent /*event*/)
{
    onAutoTextChanged(static_cast<CCControlSlider*>(sender));
}
