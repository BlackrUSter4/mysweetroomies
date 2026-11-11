// StartGameScene: the title screen's scene (StartGameSceneLayer::scene() builds it). Owns the sound engine while
// the title is up: splash music on entry, SoundEngine released on exit.
#ifndef STARTGAMESCENE_H
#define STARTGAMESCENE_H

#include "cocos2d.h"

class StartGameScene : public cocos2d::CCScene {
public:
    static StartGameScene* create();
    virtual ~StartGameScene();

    // SoundEngine::prepare() with AppConfigData-backed volume IO, then the splash BGM.
    virtual void onEnterTransitionDidFinish();
    virtual void onExit();  // SoundEngine::release()
};

#endif
