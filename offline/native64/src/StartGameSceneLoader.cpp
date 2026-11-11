#include "StartGameSceneLoader.h"

USING_NS_CC;
USING_NS_CC_EXT;

// orig 001cac08
StartGameSceneLoader::~StartGameSceneLoader() {}

// orig 001caf94
StartGameSceneLoader* StartGameSceneLoader::loader()
{
    StartGameSceneLoader* loader = new StartGameSceneLoader();
    loader->autorelease();
    return loader;
}

// orig 001caf8c
StartGameSceneLayer* StartGameSceneLoader::createCCNode(CCNode* /*parent*/, CCBReader* /*reader*/)
{
    return StartGameSceneLayer::create();
}
