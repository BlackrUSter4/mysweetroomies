#include "BeforeScene.h"

#include "ConversationScene.h"
#include "DebugSceneLayer.h"
#include "LoadingSceneLayer.h"
#include "NoticeSceneLayer.h"
#include "OptionMenuScene.h"
#include "SceneConnector.h"

USING_NS_CC;

// orig 001c584c
BeforeScene *BeforeScene::create()
{
    BeforeScene *scene = new BeforeScene();
    if (!scene->init()) {
        delete scene;
        return nullptr;
    }
    scene->autorelease();
    return scene;
}

// orig 001c83b0
BeforeScene::~BeforeScene() {}

// orig 001c854c
bool BeforeScene::init()
{
    if (!CCScene::init()) return false;
    m_nextScene = AvailableScene_StartGame;
    addChild(LoadingSceneLayer::create());
    return true;
}

// orig 001c83a8
void BeforeScene::onEnter()
{
    CCScene::onEnter();
}

// orig 001c83e0
void BeforeScene::onEnterTransitionDidFinish()
{
    CCScene::onEnterTransitionDidFinish();
    SceneConnector::getInstance()->onScenePrepared();
}

// orig 001c8574
void BeforeScene::scheduleNextScene(AvailableScene scene)
{
    m_nextScene = scene;
    scheduleOnce(schedule_selector(BeforeScene::onNextScene), 0.01f);
}

// orig 001c8514
void BeforeScene::onNextScene(float dt)
{
    CCScene *next = nullptr;
    switch (m_nextScene) {
    case AvailableScene_StartGame:  // the title screen is reached through the notice screen
    case AvailableScene_Notice:
        next = NoticeSceneLayer::scene();
        break;
    case AvailableScene_Adventure:
        next = ConversationScene::create();
        break;
    case AvailableScene_OptionMenu:
        next = OptionMenuScene::create();
        break;
    case AvailableScene_Debug:
        next = DebugSceneLayer::scene();
        break;
    default:
        return;
    }
    CCDirector::sharedDirector()->replaceScene(next);
}
