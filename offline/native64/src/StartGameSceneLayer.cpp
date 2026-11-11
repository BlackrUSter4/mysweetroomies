#include "StartGameSceneLayer.h"

#include <cstring>

#include "AfterScene.h"
#include "AppConfigData.h"
#include "CCBReader/CCBReader.h"
#include "CCBReader/CCNodeLoaderLibrary.h"
#include "DebugSceneLayer.h"
#include "OptionsDialog.h"
#include "SceneConnector.h"
#include "SoundEngine.h"
#include "StartGameScene.h"
#include "StartGameSceneLoader.h"
#include "UiHelpers.h"

USING_NS_CC;
USING_NS_CC_EXT;

namespace {

const char* const kCcbFile = "Application/StartGameSceneLayer.ccbi";
const char* const kSplashBgm = "Application/sound/bgm/splash/13_01.ogg";
const float kTransitionDuration = 1.5f;

// GUESS: what each node is (the original only has the numbers); the pairs are ccbi containers whose child is
// scaled to 85 % / 10 % of the window width, which matches the large and small title buttons.
enum Tag {
    kTagArtTop = 1,
    kTagArtBottom = 2,
    kTagParticles = 3,
    kTagCanvasArt = 4,
    kTagButton1 = 10,
    kTagButton1Image = 11,
    kTagButton2 = 12,  // hidden at start
    kTagButton2Image = 13,
    kTagButton3 = 14,
    kTagButton3Image = 15,
    kTagButton4 = 16,
    kTagButton4Image = 17,
};

// Particle effects and where they start, as fractions of the window size: two feather drifts rising from just
// below the bottom edge and a sparkle burst in the centre.
struct ParticleSpec {
    const char* plist;
    float xRatio;
    float yRatio;
};
const ParticleSpec kParticles[] = {
    {"ccbResources/start_game_scene_layer/feather.plist", 0.5f, -0.05f},
    {"ccbResources/start_game_scene_layer/feather_small.plist", 0.5f, -0.05f},
    {"ccbResources/start_game_scene_layer/kirakira.plist", 0.5f, 0.5f},
};

}  // namespace

// orig 001caf04
StartGameSceneLayer* StartGameSceneLayer::create()
{
    StartGameSceneLayer* layer = new StartGameSceneLayer();
    if (!layer->init()) {
        delete layer;
        return nullptr;
    }
    layer->autorelease();
    return layer;
}

// orig 001cb344
CCScene* StartGameSceneLayer::scene()
{
    StartGameScene* scene = StartGameScene::create();
    CCNodeLoaderLibrary* library = CCNodeLoaderLibrary::sharedCCNodeLoaderLibrary();
    library->registerCCNodeLoader("StartGameSceneLayer", StartGameSceneLoader::loader());
    CCBReader* reader = new CCBReader(library, nullptr, nullptr, nullptr);
    reader->autorelease();
    // The original assumed the ccbi loaded; a missing file leaves the scene empty here instead of crashing.
    StartGameSceneLayer* layer = dynamic_cast<StartGameSceneLayer*>(reader->readNodeGraphFromFile(kCcbFile));
    if (layer) {
        layer->initLayout();
        scene->addChild(layer);
    }
    return scene;
}

// orig 001cac38
StartGameSceneLayer::~StartGameSceneLayer() {}

// orig 001cb27c
void StartGameSceneLayer::initLayout()
{
    matchParentWidth(getChildByTag(kTagArtTop), 1.0f);
    matchParentWidth(getChildByTag(kTagArtBottom), 1.0f);
    matchParentWidthFromCanvasSize(getChildByTag(kTagCanvasArt), 1.0f);

    matchParentWidth(getChildByTag(kTagButton1)->getChildByTag(kTagButton1Image), 0.85f);
    CCNode* button2 = getChildByTag(kTagButton2);
    matchParentWidth(button2->getChildByTag(kTagButton2Image), 0.85f);
    button2->setVisible(false);
    matchParentWidth(getChildByTag(kTagButton3)->getChildByTag(kTagButton3Image), 0.1f);
    matchParentWidth(getChildByTag(kTagButton4)->getChildByTag(kTagButton4Image), 0.1f);
}

// orig 001cb26c
void StartGameSceneLayer::onEnterTransitionDidFinish()
{
    CCLayer::onEnterTransitionDidFinish();
    showParticle();
}

// orig 001cb1e0
void StartGameSceneLayer::showParticle()
{
    CCNode* container = getChildByTag(kTagParticles);
    for (const ParticleSpec& spec : kParticles) {
        CCParticleSystemQuad* particle = CCParticleSystemQuad::create(spec.plist);
        matchParentWidthFromCanvasSize(particle, 1.0f);
        CCSize winSize = CCDirector::sharedDirector()->getWinSize();
        particle->setPosition(ccp(spec.xRatio * winSize.width, spec.yRatio * winSize.height));
        container->addChild(particle);
    }
}

// orig 001cb3b4
std::string StartGameSceneLayer::getResultParam(const std::string& url)
{
    return "{\"url\":\"" + url + "\"}";
}

// orig 001cb474
void StartGameSceneLayer::onPressStartButton(CCObject* /*sender*/, unsigned int /*controlEvent*/)
{
    playButtonSe();
    std::string url = AppConfigData::getInstance()->getSiteUrl();
    SceneConnector::getInstance()->setCallbackParameter(getResultParam(url).c_str());
    CCDirector::sharedDirector()->replaceScene(
        CCTransitionTurnOffTiles::create(kTransitionDuration, AfterScene::create()));
}

// orig 001cb4e0
void StartGameSceneLayer::onPressContactButton(CCObject* /*sender*/, unsigned int /*controlEvent*/)
{
    playButtonSe();
    std::string url = AppConfigData::getInstance()->getSiteContactUrl();
    SceneConnector::getInstance()->setCallbackParameter(getResultParam(url).c_str());
    CCDirector::sharedDirector()->replaceScene(AfterScene::create());
}

// orig 001cb104
void StartGameSceneLayer::onPressDebugButton(CCObject* /*sender*/, unsigned int /*controlEvent*/)
{
    playButtonSe();
    CCDirector::sharedDirector()->replaceScene(
        CCTransitionTurnOffTiles::create(kTransitionDuration, DebugSceneLayer::scene()));
}

// orig 001cb12c
void StartGameSceneLayer::onPressOptionButton(CCObject* /*sender*/, unsigned int /*controlEvent*/)
{
    playButtonSe();
    OptionsDialog* dialog = OptionsDialog::create();
    // The dialog's samples replace the title music; bring it back once the dialog is gone.
    dialog->setDismissCallback([] { SoundEngine::getInstance()->playBgm(kSplashBgm); });
    addChild(dialog);
}

// orig 001cb3b0
void StartGameSceneLayer::onLogoAnimationStart() {}

// orig 001cabe8
void StartGameSceneLayer::onTitleShowCallback() {}

// orig 001cace8
SEL_MenuHandler StartGameSceneLayer::onResolveCCBCCMenuItemSelector(CCObject* target, const char* selectorName)
{
    if (target != this) return nullptr;
    if (std::strcmp(selectorName, "onPressStartButton") == 0) return menu_selector(StartGameSceneLayer::onStartMenuItem);
    if (std::strcmp(selectorName, "onPressDebugButton") == 0) return menu_selector(StartGameSceneLayer::onDebugMenuItem);
    if (std::strcmp(selectorName, "onPressContactButton") == 0)
        return menu_selector(StartGameSceneLayer::onContactMenuItem);
    if (std::strcmp(selectorName, "onPressOptionButton") == 0)
        return menu_selector(StartGameSceneLayer::onOptionMenuItem);
    return nullptr;
}

// orig 001cadbc
SEL_CallFuncN StartGameSceneLayer::onResolveCCBCCCallFuncSelector(CCObject* target, const char* selectorName)
{
    if (target == this && std::strcmp(selectorName, "onTitleShowCallback") == 0)
        return callfuncN_selector(StartGameSceneLayer::onTitleShowNode);
    return nullptr;
}

// orig 001cabcc
SEL_CCControlHandler StartGameSceneLayer::onResolveCCBCCControlSelector(CCObject* /*target*/,
                                                                         const char* /*selectorName*/)
{
    return nullptr;
}

void StartGameSceneLayer::onStartMenuItem(CCObject* sender) { onPressStartButton(sender, 0); }
void StartGameSceneLayer::onDebugMenuItem(CCObject* sender) { onPressDebugButton(sender, 0); }
void StartGameSceneLayer::onContactMenuItem(CCObject* sender) { onPressContactButton(sender, 0); }
void StartGameSceneLayer::onOptionMenuItem(CCObject* sender) { onPressOptionButton(sender, 0); }
void StartGameSceneLayer::onTitleShowNode(CCNode* /*sender*/) { onTitleShowCallback(); }
