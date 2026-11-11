#include "IdleScene.h"

#include "AfterScene.h"

USING_NS_CC;

// orig 001ca4c4
IdleScene::~IdleScene() {}

// orig 001ca50c
bool IdleScene::init()
{
    if (!CCScene::init()) return false;
    addChild(CCLayerColor::create(ccc4(0, 0, 0, 255)));
    return true;
}

// orig 001ca4f2
void IdleScene::onEnterTransitionDidFinish()
{
    CCScene::onEnterTransitionDidFinish();
    CCDirector::sharedDirector()->replaceScene(AfterScene::create());
}
