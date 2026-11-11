// NoticeSceneLoader: CocosBuilder loader registered as "NoticeSceneLayer".
#ifndef NOTICESCENELOADER_H
#define NOTICESCENELOADER_H

#include "CCBReader/CCLayerLoader.h"
#include "NoticeSceneLayer.h"

class NoticeSceneLoader : public cocos2d::extension::CCLayerLoader {
public:
    virtual ~NoticeSceneLoader();
    static NoticeSceneLoader* loader();

protected:
    virtual NoticeSceneLayer* createCCNode(cocos2d::CCNode* parent, cocos2d::extension::CCBReader* reader);
};

#endif
