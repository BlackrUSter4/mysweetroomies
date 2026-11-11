#include "OverlayMenuLayer.h"

#include "CCFadeInTo.h"
#include "OptionsDialog.h"
#include "RGBAColorLayer.h"
#include "SceneManager.h"
#include "SoundEngine.h"
#include "TextLogDialog.h"
#include "UiHelpers.h"

USING_NS_CC;

namespace {

const float kDesignWidth = 640.0f;
const float kFadeDuration = 0.3f;
const GLubyte kTintOpacity = 0xcc;
const float kButtonPadding = 10.0f;  // design pixels between the four buttons

const char* const kSheetPlist = "Framework/ui/conversation.plist";
const char* const kSheetTexture = "Framework/ui/conversation.png";
const char* const kStripBackground = "menu_bg.png";

enum Tag {
    kTagPopup = 0,       // child of the overlay
    kTagMenu = 3,        // child of the popup
    kTagSoundToggle = 10,  // child of the menu
};

// Toggle indices: 0 shows the "off" frames, 1 the "on" frames.
const unsigned int kToggleOff = 0;
const unsigned int kToggleOn = 1;

CCMenuItemToggle* toToggle(CCObject* sender)
{
    return sender ? dynamic_cast<CCMenuItemToggle*>(sender) : nullptr;
}

}  // namespace

// orig 001c8b2c
OverlayMenuLayer* OverlayMenuLayer::create()
{
    OverlayMenuLayer* layer = new OverlayMenuLayer();
    if (!layer->init()) {
        delete layer;
        return nullptr;
    }
    layer->autorelease();
    return layer;
}

// orig 001e13a4
OverlayMenuLayer::OverlayMenuLayer()
{
    playMenuInSE();
}

// orig 001e1328
OverlayMenuLayer::~OverlayMenuLayer()
{
    playMenuOutSE();
    // Deviation: the mute listener installed by createPopupMenuLayer() captures this layer; the original left it
    // in SoundEngine until the next prepare(), so a mute change in between would have used a dead pointer.
    SoundEngine::getInstance()->setDefaultMuteChangeListener();
}

// orig 001e1904
bool OverlayMenuLayer::init()
{
    if (!MiModalLayer::init()) return false;

    CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile(kSheetPlist, kSheetTexture);

    CCLayerColor* tint = CCLayerColor::create(ccc4(0x3d, 0xaf, 0xaa, kTintOpacity));
    tint->runAction(CCFadeInTo::create(kFadeDuration, kTintOpacity));
    addChild(tint);

    CCNode* popup = createPopupMenuLayer();
    popup->runAction(CCFadeIn::create(kFadeDuration));
    addChild(popup, 0, kTagPopup);

    m_isDialogOpen = false;
    SceneManager::getInstance()->pause();
    setDismissCallback([] {
        SoundEngine::getInstance()->play();
        SceneManager::getInstance()->resume();
    });
    return true;
}

// orig 001e1698
CCNode* OverlayMenuLayer::createPopupMenuLayer()
{
    CCSize size = getContentSize();
    float scale = size.width / kDesignWidth;

    RGBAColorLayer* popup = RGBAColorLayer::create();

    CCSprite* strip = CCSprite::createWithSpriteFrameName(kStripBackground);
    strip->setScale(scale);
    strip->setPosition(ccp(size.width * 0.5f, size.height / 3.0f));
    popup->addChild(strip);

    CCMenuItemSprite* logItem =
        createMenuItemSprite("btn_log.png", "btn_log_active.png", menu_selector(OverlayMenuLayer::onLogClicked), scale);
    CCMenuItemSprite* optionItem = createMenuItemSprite("btn_option.png", "btn_option_active.png",
                                                        menu_selector(OverlayMenuLayer::onOptionMenuClicked), scale);
    CCMenuItemToggle* autoToggle = createMenuItemToggle(
        "btn_auto_off.png", "btn_auto_off_active.png", "btn_auto.png", "btn_auto_active.png",
        menu_selector(OverlayMenuLayer::onAutoTextSwitchClicked), SceneManager::getInstance()->isAutoPlay());
    autoToggle->setCascadeOpacityEnabled(true);
    // "on" = sound on, so the toggle starts on index 1 unless muted.
    CCMenuItemToggle* soundToggle = createMenuItemToggle(
        "btn_mute_off.png", "btn_mute_off_active.png", "btn_mute_on.png", "btn_mute_on_active.png",
        menu_selector(OverlayMenuLayer::onSoundSwitchClicked), !SoundEngine::getInstance()->isMute());
    soundToggle->setCascadeOpacityEnabled(true);
    soundToggle->setTag(kTagSoundToggle);

    CCMenu* menu = CCMenu::create(logItem, optionItem, autoToggle, soundToggle, nullptr);
    menu->setTouchPriority(MiModalLayer::kModalLayerPriority - 1);
    menu->alignItemsHorizontallyWithPadding(scale * kButtonPadding);
    menu->setPosition(strip->getPosition());
    menu->setTag(kTagMenu);
    popup->addChild(menu);

    // Muting can also happen from the OptionsDialog opened from here; keep the toggle in step.
    SoundEngine::getInstance()->setMuteChangeListener([this](bool muted) {
        CCNode* toggle = getChildByTag(kTagPopup)->getChildByTag(kTagMenu)->getChildByTag(kTagSoundToggle);
        static_cast<CCMenuItemToggle*>(toggle)->setSelectedIndex(muted ? kToggleOff : kToggleOn);
    });
    return popup;
}

// orig 001e15a6
CCMenuItemSprite* OverlayMenuLayer::createMenuItemSprite(const std::string& normalFrame,
                                                         const std::string& selectedFrame, SEL_MenuHandler selector,
                                                         float scale)
{
    CCSprite* normal = CCSprite::createWithSpriteFrameName(normalFrame.c_str());
    CCSprite* selected = CCSprite::createWithSpriteFrameName(selectedFrame.c_str());
    CCMenuItemSprite* item = selector ? CCMenuItemSprite::create(normal, selected, this, selector)
                                      : CCMenuItemSprite::create(normal, selected, nullptr);
    item->setScale(scale);
    return item;
}

// orig 001e15fc
CCMenuItemToggle* OverlayMenuLayer::createMenuItemToggle(const std::string& offNormalFrame,
                                                         const std::string& offSelectedFrame,
                                                         const std::string& onNormalFrame,
                                                         const std::string& onSelectedFrame, SEL_MenuHandler selector,
                                                         bool selectOn)
{
    // The sub-items have no target of their own: the toggle reports the click.
    CCMenuItemSprite* off = createMenuItemSprite(offNormalFrame, offSelectedFrame, nullptr, 1.0f);
    CCMenuItemSprite* on = createMenuItemSprite(onNormalFrame, onSelectedFrame, nullptr, 1.0f);
    CCMenuItemToggle* toggle = CCMenuItemToggle::createWithTarget(this, selector, off, on, nullptr);
    toggle->setScale(CCDirector::sharedDirector()->getWinSize().width / kDesignWidth);
    if (selectOn) toggle->setSelectedIndex(kToggleOn);
    return toggle;
}

// orig 001e1416
void OverlayMenuLayer::showMenuLayer()
{
    playMenuOutSE();
    getChildByTag(kTagPopup)->runAction(CCSequence::create(CCShow::create(), nullptr));
}

// orig 001e13ec
void OverlayMenuLayer::hideMenuLayer()
{
    playMenuInSE();
    getChildByTag(kTagPopup)->runAction(CCSequence::create(CCHide::create(), nullptr));
}

// orig 001e1468
void OverlayMenuLayer::showLogDialog()
{
    hideMenuLayer();
    m_isDialogOpen = true;
    TextLogDialog* dialog = TextLogDialog::create();
    dialog->setDismissCallback([this] {
        showMenuLayer();
        m_isDialogOpen = false;
    });
    addChild(dialog);
}

// orig 001e1508
void OverlayMenuLayer::showOptionDialog()
{
    hideMenuLayer();
    m_isDialogOpen = true;
    OptionsDialog* dialog = OptionsDialog::create();
    dialog->setDismissCallback([this] {
        showMenuLayer();
        m_isDialogOpen = false;
    });
    addChild(dialog);
}

// orig 001e14f4
void OverlayMenuLayer::onLogClicked(CCObject* /*sender*/)
{
    if (!m_isDialogOpen) showLogDialog();
}

// orig 001e1594
void OverlayMenuLayer::onOptionMenuClicked(CCObject* /*sender*/)
{
    if (!m_isDialogOpen) showOptionDialog();
}

// orig 001e19f0
void OverlayMenuLayer::onAutoTextSwitchClicked(CCObject* sender)
{
    bool autoPlay = SceneManager::getInstance()->isAutoPlay();
    // The original dereferenced the cast result unchecked.
    CCMenuItemToggle* toggle = toToggle(sender);
    if (!toggle) return;
    unsigned int index = toggle->getSelectedIndex();
    if (index == kToggleOff) {
        if (autoPlay) {
            SceneManager::getInstance()->setAutoPlay(false);
            playMenuOutSE();
        }
    } else if (index == kToggleOn && !autoPlay) {
        SceneManager::getInstance()->setAutoPlay(true);
        playMenuInSE();
    }
}

// orig 001e12c8
void OverlayMenuLayer::onSoundSwitchClicked(CCObject* sender)
{
    // The original dereferenced the cast result unchecked.
    CCMenuItemToggle* toggle = toToggle(sender);
    if (!toggle) return;
    unsigned int index = toggle->getSelectedIndex();
    if (index == kToggleOff) {
        SoundEngine::getInstance()->setMute();
    } else if (index == kToggleOn) {
        SoundEngine::getInstance()->resetMute();
        playMenuInSE();
    }
}
