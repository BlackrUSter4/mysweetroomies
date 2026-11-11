#ifndef LOADINGSCENELAYER_H
#define LOADINGSCENELAYER_H

#include "cocos2d.h"

// The looping 9-frame loading animation shown centred while BeforeScene/AfterScene wait.
class LoadingSceneLayer : public cocos2d::CCLayer {
public:
    static LoadingSceneLayer* create();
    virtual ~LoadingSceneLayer();
    virtual bool init();  // CCLayer::init, then initUI()

    void initUI();
};

#endif
