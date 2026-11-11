#include "NoticeScene.h"

#include "StartGameSceneLayer.h"

USING_NS_CC;

namespace {

const float kNoticeDuration = 2.6f;      // seconds the notice stays up
const float kTransitionDuration = 1.5f;  // CCTransitionFadeTR to the title

}  // namespace

// orig 001ca6cc
NoticeScene* NoticeScene::create()
{
    NoticeScene* scene = new NoticeScene();
    if (!scene->init()) {
        delete scene;
        return nullptr;
    }
    scene->autorelease();
    return scene;
}

// orig 001ca67c
NoticeScene::~NoticeScene() {}

// orig 001ca658
void NoticeScene::onEnterTransitionDidFinish()
{
    CCScene::onEnterTransitionDidFinish();
    scheduleOnce(schedule_selector(NoticeScene::onNext), kNoticeDuration);
}

// orig 001ca6aa
void NoticeScene::onNext(float /*dt*/)
{
    CCDirector::sharedDirector()->replaceScene(
        CCTransitionFadeTR::create(kTransitionDuration, StartGameSceneLayer::scene()));
}
