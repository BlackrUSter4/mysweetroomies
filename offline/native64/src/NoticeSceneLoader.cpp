#include "NoticeSceneLoader.h"

USING_NS_CC;
USING_NS_CC_EXT;

// orig 001ca578
NoticeSceneLoader::~NoticeSceneLoader() {}

// orig 001ca7ac
NoticeSceneLoader* NoticeSceneLoader::loader()
{
    NoticeSceneLoader* loader = new NoticeSceneLoader();
    loader->autorelease();
    return loader;
}

// orig 001ca7a4
NoticeSceneLayer* NoticeSceneLoader::createCCNode(CCNode* /*parent*/, CCBReader* /*reader*/)
{
    return NoticeSceneLayer::create();
}
