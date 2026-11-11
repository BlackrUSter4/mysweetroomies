// NoticeScene: the scene NoticeSceneLayer::scene() builds. The notice stays up 2.6 s, then the title screen
// (StartGameSceneLayer::scene()) replaces it with a 1.5 s CCTransitionFadeTR (tiles fading bottom-left to
// top-right).
#ifndef NOTICESCENE_H
#define NOTICESCENE_H

#include "cocos2d.h"

class NoticeScene : public cocos2d::CCScene {
public:
    static NoticeScene* create();
    virtual ~NoticeScene();

    virtual void onEnterTransitionDidFinish();  // schedules onNext once
    void onNext(float dt);
};

#endif
