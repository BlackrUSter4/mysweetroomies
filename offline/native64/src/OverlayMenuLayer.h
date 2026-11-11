// OverlayMenuLayer: the in-story pause menu ConversationScene adds when the menu button is pressed. A tinted
// full-screen CCLayerColor fades in under a strip of four buttons (Framework/ui/conversation.plist):
//
//   log        -> TextLogDialog          option -> OptionsDialog
//   auto-play  toggle (SceneManager)     sound  -> mute toggle (SoundEngine), child tag 10 of the menu
//
// The story is paused while the overlay is up; tapping outside the buttons dismisses it, which resumes the
// story and replays the current sound. While one of the two dialogs is open the strip is hidden and the log /
// option buttons are ignored.
//
// Children: 0 the popup (RGBAColorLayer) holding the background sprite and the CCMenu (tag 3).
#ifndef OVERLAYMENULAYER_H
#define OVERLAYMENULAYER_H

#include <string>

#include "MiModalLayer.h"
#include "cocos2d.h"

class OverlayMenuLayer : public MiModalLayer {
public:
    static OverlayMenuLayer* create();
    OverlayMenuLayer();           // plays the menu-in click
    virtual ~OverlayMenuLayer();  // plays the menu-out click
    virtual bool init();

    // The tinted popup with the button strip (returned, not yet added).
    cocos2d::CCNode* createPopupMenuLayer();
    // Sprite button from two frame names, scaled by `scale`; without a selector it has no target.
    cocos2d::CCMenuItemSprite* createMenuItemSprite(const std::string& normalFrame, const std::string& selectedFrame,
                                                    cocos2d::SEL_MenuHandler selector, float scale);
    // Two-state toggle (index 0 = off frames, 1 = on frames) targeting this; starts on index 1 when `selectOn`.
    cocos2d::CCMenuItemToggle* createMenuItemToggle(const std::string& offNormalFrame,
                                                    const std::string& offSelectedFrame,
                                                    const std::string& onNormalFrame,
                                                    const std::string& onSelectedFrame,
                                                    cocos2d::SEL_MenuHandler selector, bool selectOn);

    void showMenuLayer();  // shows the popup again (after a dialog closed)
    void hideMenuLayer();  // hides the popup while a dialog is open
    void showLogDialog();
    void showOptionDialog();

    void onLogClicked(cocos2d::CCObject* sender);
    void onOptionMenuClicked(cocos2d::CCObject* sender);
    void onAutoTextSwitchClicked(cocos2d::CCObject* sender);  // sender: the CCMenuItemToggle
    void onSoundSwitchClicked(cocos2d::CCObject* sender);     // sender: the CCMenuItemToggle

private:
    bool m_isDialogOpen = false;  // orig +0x11c: TextLogDialog or OptionsDialog on top
};

#endif
