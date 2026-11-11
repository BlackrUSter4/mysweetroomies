// StartGameSceneLayer: the title screen, loaded from Application/StartGameSceneLayer.ccbi. Its CocosBuilder menu
// items call onPressStartButton / onPressDebugButton / onPressContactButton / onPressOptionButton; the title
// timeline calls onTitleShowCallback. Three particle systems (feathers, sparkles) start once it is on screen.
//
// Child tags from the ccbi: 1 and 2 full-width art, 3 particle container, 4 art scaled like the canvas, and the
// parent/child pairs 10/11, 12/13 (12 hidden), 14/15, 16/17.
#ifndef STARTGAMESCENELAYER_H
#define STARTGAMESCENELAYER_H

#include <string>

#include "CCBReader/CCBSelectorResolver.h"
#include "cocos2d.h"

class StartGameSceneLayer : public cocos2d::CCLayer, public cocos2d::extension::CCBSelectorResolver {
public:
    static StartGameSceneLayer* create();
    // StartGameScene holding the layer read from the ccbi (layout fixed up by initLayout()).
    static cocos2d::CCScene* scene();
    virtual ~StartGameSceneLayer();

    void initLayout();
    virtual void onEnterTransitionDidFinish();  // showParticle()
    void showParticle();

    // {"url":"<url>"}: the callback parameter Java receives with the scene end.
    std::string getResultParam(const std::string& url);

    // Leave the title through AfterScene; Java opens the given page.
    void onPressStartButton(cocos2d::CCObject* sender, unsigned int controlEvent);    // site top page
    void onPressContactButton(cocos2d::CCObject* sender, unsigned int controlEvent);  // contact page
    void onPressDebugButton(cocos2d::CCObject* sender, unsigned int controlEvent);    // DebugSceneLayer
    void onPressOptionButton(cocos2d::CCObject* sender, unsigned int controlEvent);   // OptionsDialog on top

    void onLogoAnimationStart();  // empty in the original
    void onTitleShowCallback();   // empty in the original (ccbi timeline callback)

    // CCBSelectorResolver
    virtual cocos2d::SEL_MenuHandler onResolveCCBCCMenuItemSelector(cocos2d::CCObject* target,
                                                                    const char* selectorName);
    virtual cocos2d::SEL_CallFuncN onResolveCCBCCCallFuncSelector(cocos2d::CCObject* target,
                                                                  const char* selectorName);
    virtual cocos2d::extension::SEL_CCControlHandler onResolveCCBCCControlSelector(cocos2d::CCObject* target,
                                                                                    const char* selectorName);

private:
    // Menu items call a SEL_MenuHandler (one argument); the original cast the two-argument handlers above to that
    // type. These forward with controlEvent 0 instead.
    void onStartMenuItem(cocos2d::CCObject* sender);
    void onDebugMenuItem(cocos2d::CCObject* sender);
    void onContactMenuItem(cocos2d::CCObject* sender);
    void onOptionMenuItem(cocos2d::CCObject* sender);
    void onTitleShowNode(cocos2d::CCNode* sender);  // SEL_CallFuncN adapter for onTitleShowCallback()
};

#endif
