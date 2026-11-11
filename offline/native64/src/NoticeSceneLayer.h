// NoticeSceneLayer: the notice / caution screen shown before the title, loaded from
// Application/NoticeSceneLayer.ccbi. No buttons; NoticeScene moves on by itself.
#ifndef NOTICESCENELAYER_H
#define NOTICESCENELAYER_H

#include "CCBReader/CCBSelectorResolver.h"
#include "cocos2d.h"

class NoticeSceneLayer : public cocos2d::CCLayer, public cocos2d::extension::CCBSelectorResolver {
public:
    static NoticeSceneLayer* create();
    static cocos2d::CCScene* scene();  // NoticeScene holding the layer read from the ccbi
    virtual ~NoticeSceneLayer();

    void initLayout();  // children 0 and 1 scaled to the window width
    virtual void onEnterTransitionDidFinish();

    // CCBSelectorResolver: the ccbi binds nothing.
    virtual cocos2d::SEL_MenuHandler onResolveCCBCCMenuItemSelector(cocos2d::CCObject* target,
                                                                    const char* selectorName);
    virtual cocos2d::extension::SEL_CCControlHandler onResolveCCBCCControlSelector(cocos2d::CCObject* target,
                                                                                    const char* selectorName);
};

#endif
