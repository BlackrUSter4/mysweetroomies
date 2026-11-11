#include "NoticeSceneLayer.h"

#include "CCBReader/CCBReader.h"
#include "CCBReader/CCNodeLoaderLibrary.h"
#include "NoticeScene.h"
#include "NoticeSceneLoader.h"
#include "UiHelpers.h"

USING_NS_CC;
USING_NS_CC_EXT;

namespace {

const char* const kCcbFile = "Application/NoticeSceneLayer.ccbi";

}  // namespace

// orig 001ca71c
NoticeSceneLayer* NoticeSceneLayer::create()
{
    NoticeSceneLayer* layer = new NoticeSceneLayer();
    if (!layer->init()) {
        delete layer;
        return nullptr;
    }
    layer->autorelease();
    return layer;
}

// orig 001ca850
CCScene* NoticeSceneLayer::scene()
{
    NoticeScene* scene = NoticeScene::create();
    CCNodeLoaderLibrary* library = CCNodeLoaderLibrary::sharedCCNodeLoaderLibrary();
    library->registerCCNodeLoader("NoticeSceneLayer", NoticeSceneLoader::loader());
    CCBReader* reader = new CCBReader(library, nullptr, nullptr, nullptr);
    reader->autorelease();
    // The original assumed the ccbi loaded; without it the scene stays empty (and still moves on to the title).
    NoticeSceneLayer* layer = dynamic_cast<NoticeSceneLayer*>(reader->readNodeGraphFromFile(kCcbFile));
    if (layer) {
        layer->initLayout();
        scene->addChild(layer);
    }
    return scene;
}

// orig 001ca5a8
NoticeSceneLayer::~NoticeSceneLayer() {}

// orig 001ca824
void NoticeSceneLayer::initLayout()
{
    matchParentWidth(getChildByTag(0), 1.0f);
    matchParentWidth(getChildByTag(1), 1.0f);
}

// orig 001ca570
void NoticeSceneLayer::onEnterTransitionDidFinish()
{
    CCLayer::onEnterTransitionDidFinish();
}

// orig 001ca538
SEL_MenuHandler NoticeSceneLayer::onResolveCCBCCMenuItemSelector(CCObject* /*target*/, const char* /*selectorName*/)
{
    return nullptr;
}

// orig 001ca554
SEL_CCControlHandler NoticeSceneLayer::onResolveCCBCCControlSelector(CCObject* /*target*/,
                                                                      const char* /*selectorName*/)
{
    return nullptr;
}
