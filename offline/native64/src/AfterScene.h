// AfterScene: the scene a finished top-level scene (story end, title screen exit, debug menu) is replaced with. It
// frees unused textures, shows a LoadingSceneLayer and, 0.5 s after it is on screen, tells Java through
// SceneConnector::onSceneEnd() which scene ended; Java then decides what to start next.
#ifndef AFTERSCENE_H
#define AFTERSCENE_H

#include "cocos2d.h"

class AfterScene : public cocos2d::CCScene {
public:
    static AfterScene *create();
    virtual ~AfterScene();
    virtual bool init();
    virtual void onEnter();
    virtual void onEnterTransitionDidFinish();

    void delayScene();  // SceneConnector::getInstance()->onSceneEnd()

private:
    void onDelayElapsed(float dt);  // scheduleOnce target; forwards to delayScene()
};

#endif
