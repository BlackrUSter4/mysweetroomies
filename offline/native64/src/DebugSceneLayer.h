// DebugSceneLayer: the developer screen, loaded from Application/DebugSceneLayer.ccbi. Two EnvTableLayers share
// the ccbi's container (child tag 10):
//   tag 11  the current environment and its URLs; tapping "jump to debug page" leaves through AfterScene with
//           that URL as the callback parameter
//   tag 12  every environment of AppConfigData; tapping one switches to it (and commits), then shows table 11
// The footer (tag 20) holds the title label (21), the close button (22 -> item 23) and the list button
// (24 -> item 25), which flips between the two tables.
#ifndef DEBUGSCENELAYER_H
#define DEBUGSCENELAYER_H

#include <string>

#include "CCBReader/CCBSelectorResolver.h"
#include "cocos2d.h"

class EnvTableLayer;

class DebugSceneLayer : public cocos2d::CCLayer, public cocos2d::extension::CCBSelectorResolver {
public:
    static DebugSceneLayer* create();
    static cocos2d::CCScene* scene();  // plain CCScene holding the layer read from the ccbi
    virtual ~DebugSceneLayer();

    void initLayout();  // builds both tables, wires the footer buttons, fills the tables
    EnvTableLayer* getTable(int tag);
    void updateView();   // refills both tables from AppConfigData
    void switchTable();  // shows the hidden table (with touches), hides the other, refills

    void onPressListButton(cocos2d::CCObject* sender, unsigned int controlEvent);   // switchTable()
    void onPressCloseButton(cocos2d::CCObject* sender, unsigned int controlEvent);  // exitScene("")
    // Ends the debug scene: scene name "debug", callback parameter {"url":"<url>"} unless url is empty.
    void exitScene(const std::string& url);

    // CCBSelectorResolver: the ccbi binds nothing (the menu resolver only logs the name).
    virtual cocos2d::SEL_MenuHandler onResolveCCBCCMenuItemSelector(cocos2d::CCObject* target,
                                                                    const char* selectorName);
    virtual cocos2d::extension::SEL_CCControlHandler onResolveCCBCCControlSelector(cocos2d::CCObject* target,
                                                                                    const char* selectorName);

private:
    // CCMenuItem targets (one argument) forwarding to the two-argument handlers; the original cast the handlers.
    void onListMenuItem(cocos2d::CCObject* sender);
    void onCloseMenuItem(cocos2d::CCObject* sender);
};

#endif
