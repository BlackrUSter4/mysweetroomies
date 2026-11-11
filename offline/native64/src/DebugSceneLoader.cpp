#include "DebugSceneLoader.h"

USING_NS_CC;
USING_NS_CC_EXT;

// orig 001c9ac0
DebugSceneLoader::~DebugSceneLoader() {}

// orig 001c9ce0
DebugSceneLoader* DebugSceneLoader::loader()
{
    DebugSceneLoader* loader = new DebugSceneLoader();
    loader->autorelease();
    return loader;
}

// orig 001c9cd8
DebugSceneLayer* DebugSceneLoader::createCCNode(CCNode* /*parent*/, CCBReader* /*reader*/)
{
    return DebugSceneLayer::create();
}
