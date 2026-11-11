#include "LoadingSceneLayer.h"

USING_NS_CC;

namespace {

const char* const kAnimationFrames[] = {
    "Framework/ui/scene_loading/loading-anime-00.png", "Framework/ui/scene_loading/loading-anime-01.png",
    "Framework/ui/scene_loading/loading-anime-02.png", "Framework/ui/scene_loading/loading-anime-03.png",
    "Framework/ui/scene_loading/loading-anime-04.png", "Framework/ui/scene_loading/loading-anime-05.png",
    "Framework/ui/scene_loading/loading-anime-06.png", "Framework/ui/scene_loading/loading-anime-07.png",
    "Framework/ui/scene_loading/loading-anime-08.png",
};

const float kFrameDelay = 0.111f;
// Artwork is sized for a 640-pixel-wide screen.
const float kDesignWidth = 640.0f;

}  // namespace

LoadingSceneLayer* LoadingSceneLayer::create() {
    LoadingSceneLayer* layer = new LoadingSceneLayer();
    if (layer->init()) {
        layer->autorelease();
        return layer;
    }
    delete layer;
    return nullptr;
}

LoadingSceneLayer::~LoadingSceneLayer() {}

bool LoadingSceneLayer::init() {
    if (!CCLayer::init()) return false;
    initUI();
    return true;
}

// orig 001c8124
void LoadingSceneLayer::initUI() {
    CCAnimation* animation = CCAnimation::create();
    for (const char* frame : kAnimationFrames) animation->addSpriteFrameWithFileName(frame);
    animation->setDelayPerUnit(kFrameDelay);
    animation->setLoops(static_cast<unsigned int>(-1));  // forever

    CCSprite* sprite = CCSprite::create(kAnimationFrames[0]);
    sprite->setAnchorPoint(ccp(0.5f, 0.5f));
    CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    sprite->setPosition(ccp(winSize.width * 0.5f, winSize.height * 0.5f));
    sprite->setScale(CCDirector::sharedDirector()->getWinSize().width / kDesignWidth);
    sprite->runAction(CCAnimate::create(animation));
    addChild(sprite);
}
