#include "AfterScene.h"

#include "LoadingSceneLayer.h"
#include "SceneConnector.h"

USING_NS_CC;

// orig 001c8cfc
AfterScene *AfterScene::create()
{
    AfterScene *scene = new AfterScene();
    if (!scene->init()) {
        delete scene;
        return nullptr;
    }
    scene->autorelease();
    return scene;
}

// orig 001c8244
AfterScene::~AfterScene() {}

// orig 001c8380
bool AfterScene::init()
{
    if (!CCScene::init()) return false;
    // The scene that just ended is gone; drop its textures before Java starts the next one.
    CCTextureCache::sharedTextureCache()->removeUnusedTextures();
    addChild(LoadingSceneLayer::create());
    return true;
}

// orig 001c821a
void AfterScene::onEnter()
{
    CCScene::onEnter();
}

// orig 001c8224
void AfterScene::onEnterTransitionDidFinish()
{
    CCScene::onEnterTransitionDidFinish();
    // The original scheduled delayScene() itself, cast to a SEL_SCHEDULE.
    scheduleOnce(schedule_selector(AfterScene::onDelayElapsed), 0.5f);
}

void AfterScene::onDelayElapsed(float dt)
{
    delayScene();
}

// orig 001c8274
void AfterScene::delayScene()
{
    SceneConnector::getInstance()->onSceneEnd();
}
