#include "AppMenuLayer.h"

USING_NS_CC;

// orig 001c8d60
AppMenuLayer* AppMenuLayer::create()
{
    AppMenuLayer* layer = new AppMenuLayer();
    if (!layer->init()) {
        delete layer;
        return nullptr;
    }
    layer->autorelease();
    return layer;
}

// orig 001c8694
AppMenuLayer::~AppMenuLayer() {}

// orig 001de1fc
AppMenuLayer* AppMenuLayer::setAppSetting(SEL_MenuHandler /*voiceSelector*/, SEL_MenuHandler menuPopSelector)
{
    prepare("Framework/ui/conversation.plist", "Framework/ui/conversation.png");
    setMenuPopButton("btn_menu.png", "btn_menu_active.png", menuPopSelector);
    return this;
}
