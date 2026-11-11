// BeforeScene: the idle "loading" scene every top-level scene switch goes through (see SceneConnector.h).
//
//   AppDelegate / SceneConnector::startScene -> replaceScene(BeforeScene::create())
//   BeforeScene::onEnterTransitionDidFinish   -> SceneConnector::onScenePrepared()
//   SceneConnector                            -> scheduleNextScene(scene): 0.01 s later onNextScene() replaces this
//                                                scene with NoticeSceneLayer::scene() (start_game and notice),
//                                                ConversationScene, OptionMenuScene or DebugSceneLayer::scene().
// It shows a LoadingSceneLayer meanwhile.
#ifndef BEFORESCENE_H
#define BEFORESCENE_H

#include "ExEnums.h"
#include "cocos2d.h"

class BeforeScene : public cocos2d::CCScene {
public:
    static BeforeScene *create();
    virtual ~BeforeScene();
    virtual bool init();
    virtual void onEnter();
    virtual void onEnterTransitionDidFinish();

    // Remembers `scene` and calls onNextScene() once, 0.01 s from now.
    void scheduleNextScene(AvailableScene scene);
    void onNextScene(float dt);

private:
    AvailableScene m_nextScene = AvailableScene_StartGame;
};

#endif
