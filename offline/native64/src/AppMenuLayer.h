// AppMenuLayer: the story screen's menu button (btn_menu.png from Framework/ui/conversation.plist).
//
//   addChild(AppMenuLayer::create()->setAppSetting(menu_selector(A::onVoice), menu_selector(A::onMenuPopup)));
//
// As in the original, only the second selector is used (the voice-replay button is never created) and the
// button's target is the AppMenuLayer itself: the handler runs with `this` pointing at this layer, so it may only
// use CCNode behaviour through `this` (ConversationScene::onTouchMenuPopup just calls addChild()).
#ifndef APPMENULAYER_H
#define APPMENULAYER_H

#include "MenuLayer.h"
#include "cocos2d.h"

class AppMenuLayer : public MenuLayer {
public:
    static AppMenuLayer* create();
    virtual ~AppMenuLayer();

    // Loads the conversation sprite sheet and adds the menu-popup button; returns this.
    AppMenuLayer* setAppSetting(cocos2d::SEL_MenuHandler voiceSelector, cocos2d::SEL_MenuHandler menuPopSelector);
};

#endif
