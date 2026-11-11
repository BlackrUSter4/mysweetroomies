// DebugSceneLoader: CocosBuilder loader registered as "DebugSceneLayer".
#ifndef DEBUGSCENELOADER_H
#define DEBUGSCENELOADER_H

#include "CCBReader/CCLayerLoader.h"
#include "DebugSceneLayer.h"

class DebugSceneLoader : public cocos2d::extension::CCLayerLoader {
public:
    virtual ~DebugSceneLoader();
    static DebugSceneLoader* loader();

protected:
    virtual DebugSceneLayer* createCCNode(cocos2d::CCNode* parent, cocos2d::extension::CCBReader* reader);
};

#endif
