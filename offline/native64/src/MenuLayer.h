// MenuLayer: base of AppMenuLayer, the always-visible story buttons. Each button is a CCMenuItemSprite in its own
// CCMenu (so it sits relative to the screen centre, CCMenu's default position) whose target is this layer.
#ifndef MENULAYER_H
#define MENULAYER_H

#include <functional>

#include "cocos2d.h"

class MenuLayer : public cocos2d::CCLayer {
public:
    virtual ~MenuLayer();
    virtual bool init();  // CCLayer::init
    void initUi();        // empty in the original

    // Loads a sprite sheet (plist + texture file) for the buttons; returns `plist`.
    const char* prepare(const char* plist, const char* texture);

    // Adds a button made of two sprite frames; `layout` places the item before it is shown. Note the item's
    // target is this MenuLayer, whatever class `selector` belongs to (as in the original).
    void setMenu(const char* normalFrame, const char* selectedFrame, cocos2d::SEL_MenuHandler selector,
                 std::function<void(cocos2d::CCMenuItemSprite*)> layout);
    // setMenu with the bottom-right / bottom-left corner layouts.
    void setVoiceButton(const char* normalFrame, const char* selectedFrame, cocos2d::SEL_MenuHandler selector);
    void setMenuPopButton(const char* normalFrame, const char* selectedFrame, cocos2d::SEL_MenuHandler selector);
};

#endif
