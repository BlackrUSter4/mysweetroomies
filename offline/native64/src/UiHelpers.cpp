#include "UiHelpers.h"

#include "GUI/CCControlExtension/CCControlSlider.h"
#include "OptionsDialog.h"
#include "SceneManager.h"
#include "SoundEngine.h"

USING_NS_CC;
USING_NS_CC_EXT;

namespace {

// Width of the screen the ui artwork was drawn for.
const float kDesignWidth = 640.0f;

// The text-speed slider maps 0..1 onto [kMinAutoPlayDelay, 2 * AUTOPLAY_DELAY - kMinAutoPlayDelay], so the
// default delay sits in the middle of the slider.
const float kMinAutoPlayDelay = 0.25f;

const char* const kButtonSe = "Application/sound/se/se-system-button.ogg";
const char* const kCloseSe = "Application/sound/se/se-system-close.ogg";

}  // namespace

// orig 001c9d1c
void scalingNode(CCNode* node)
{
    node->setScale(CCDirector::sharedDirector()->getWinSize().width / kDesignWidth);
}

// orig 001ca7e8
void matchParentWidth(CCNode* node, float ratio)
{
    CCSize contentSize = node->getContentSize();
    CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    node->setScale(winSize.width / contentSize.width * ratio);
}

// orig 001cb1b0
void matchParentWidthFromCanvasSize(CCNode* node, float ratio)
{
    node->setScale(CCDirector::sharedDirector()->getWinSize().width / kDesignWidth * ratio);
}

// orig 001cb0f0
void playButtonSe()
{
    SoundEngine::getInstance()->playSe(kButtonSe);
}

// orig 001e12b4: the same file as playButtonSe (the original kept two copies of the path).
void playMenuInSE()
{
    SoundEngine::getInstance()->playSe(kButtonSe);
}

// orig 001e1314
void playMenuOutSE()
{
    SoundEngine::getInstance()->playSe(kCloseSe);
}

// orig 001e0f10
void setAutoTextValueFromSlider(float sliderValue)
{
    float range = (SceneManager::AUTOPLAY_DELAY - kMinAutoPlayDelay) * 2;
    SceneManager::getInstance()->setAutoPlayDelay(sliderValue * range + kMinAutoPlayDelay);
}

// orig 001e0f7a
void setDefaultValue(OptionsDialog* dialog, OptionsDialogTag tag)
{
    float value = 0.0f;
    if (tag == OptionsDialogTag_Se) {
        value = SoundEngine::getInstance()->getDefaultSeValue();
    } else if (tag == OptionsDialogTag_TextSpeed) {
        value = 0.5f;
    } else if (tag == OptionsDialogTag_Bgm) {
        value = SoundEngine::getInstance()->getDefaultBgmValue();
    }

    // setValue() also fires the slider's value-changed handler; the explicit apply below repeats it, as the
    // original did.
    CCControlSlider* slider = static_cast<CCControlSlider*>(dialog->getChildByTag(tag));
    slider->setValue(value);

    if (tag == OptionsDialogTag_Se) {
        SoundEngine::getInstance()->setVolumeSe(value);
    } else if (tag == OptionsDialogTag_TextSpeed) {
        setAutoTextValueFromSlider(value);
    } else if (tag == OptionsDialogTag_Bgm) {
        SoundEngine::getInstance()->setVolumeBgm(value);
    }
}
