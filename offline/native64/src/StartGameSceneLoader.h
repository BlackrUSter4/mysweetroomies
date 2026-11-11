// StartGameSceneLoader: CocosBuilder loader registered as "StartGameSceneLayer" (the ccbi's custom class).
#ifndef STARTGAMESCENELOADER_H
#define STARTGAMESCENELOADER_H

#include "CCBReader/CCLayerLoader.h"
#include "StartGameSceneLayer.h"

class StartGameSceneLoader : public cocos2d::extension::CCLayerLoader {
public:
    virtual ~StartGameSceneLoader();
    static StartGameSceneLoader* loader();

protected:
    virtual StartGameSceneLayer* createCCNode(cocos2d::CCNode* parent, cocos2d::extension::CCBReader* reader);
};

#endif
