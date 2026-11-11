#include "DebugSceneLayer.h"

#include <vector>

#include "AfterScene.h"
#include "AppConfigData.h"
#include "CCBReader/CCBReader.h"
#include "CCBReader/CCNodeLoaderLibrary.h"
#include "DebugSceneLoader.h"
#include "EnvTableLayer.h"
#include "ExEnums.h"
#include "SceneConnector.h"
#include "UiHelpers.h"

USING_NS_CC;
USING_NS_CC_EXT;

namespace {

const char* const kCcbFile = "Application/DebugSceneLayer.ccbi";
// Row of the settings table that leaves for the web debug page.
const char* const kJumpToDebugPageKey = "jump to debug page";
const float kDesignWidth = 640.0f;

enum Tag {
    kTagTableContainer = 10,
    kTagSettingsTable = 11,
    kTagEnvironmentTable = 12,
    kTagFooter = 20,
    kTagTitleLabel = 21,
    kTagCloseButton = 22,
    kTagCloseMenuItem = 23,
    kTagListButton = 24,
    kTagListMenuItem = 25,
};

EnvTableLayer* createTable(CCNode* container, int tag, bool shown)
{
    EnvTableLayer* table = EnvTableLayer::create(container->getContentSize());
    table->setContentSize(container->getContentSize());
    table->setTag(tag);
    table->setVisible(shown);
    table->setTouchEnabled(shown);
    return table;
}

}  // namespace

// orig 001c9c50
DebugSceneLayer* DebugSceneLayer::create()
{
    DebugSceneLayer* layer = new DebugSceneLayer();
    if (!layer->init()) {
        delete layer;
        return nullptr;
    }
    layer->autorelease();
    return layer;
}

// orig 001ca23c
CCScene* DebugSceneLayer::scene()
{
    CCScene* scene = CCScene::create();
    CCNodeLoaderLibrary* library = CCNodeLoaderLibrary::sharedCCNodeLoaderLibrary();
    library->registerCCNodeLoader("DebugSceneLayer", DebugSceneLoader::loader());
    CCBReader* reader = new CCBReader(library, nullptr, nullptr, nullptr);
    reader->autorelease();
    // The original assumed the ccbi loaded; a missing file leaves the scene empty here instead of crashing.
    DebugSceneLayer* layer = dynamic_cast<DebugSceneLayer*>(reader->readNodeGraphFromFile(kCcbFile));
    if (layer) {
        layer->initLayout();
        scene->addChild(layer);
    }
    return scene;
}

// orig 001c9af0
DebugSceneLayer::~DebugSceneLayer() {}

// orig 001ca01c
void DebugSceneLayer::initLayout()
{
    CCNode* container = getChildByTag(kTagTableContainer);

    EnvTableLayer* settings = createTable(container, kTagSettingsTable, true);
    settings->setCallback([this](const std::string& key, const std::string& value) {
        if (key.compare(kJumpToDebugPageKey) == 0) exitScene(value);
    });
    container->addChild(settings);

    EnvTableLayer* environments = createTable(container, kTagEnvironmentTable, false);
    environments->setCallback([this](const std::string& env, const std::string& /*value*/) {
        AppConfigData::getInstance()->switching(env).commit();
        switchTable();
    });
    container->addChild(environments);

    CCNode* footer = getChildByTag(kTagFooter);
    CCMenuItem* closeItem =
        static_cast<CCMenuItem*>(footer->getChildByTag(kTagCloseButton)->getChildByTag(kTagCloseMenuItem));
    CCMenuItem* listItem =
        static_cast<CCMenuItem*>(footer->getChildByTag(kTagListButton)->getChildByTag(kTagListMenuItem));
    listItem->setTarget(this, menu_selector(DebugSceneLayer::onListMenuItem));
    closeItem->setTarget(this, menu_selector(DebugSceneLayer::onCloseMenuItem));
    scalingNode(closeItem);
    scalingNode(listItem);

    CCLabelTTF* title = static_cast<CCLabelTTF*>(footer->getChildByTag(kTagTitleLabel));
    float scale = CCDirector::sharedDirector()->getWinSize().width / kDesignWidth;
    title->setFontSize(title->getFontSize() * scale);

    updateView();
}

// orig 001c9d44
EnvTableLayer* DebugSceneLayer::getTable(int tag)
{
    return static_cast<EnvTableLayer*>(getChildByTag(kTagTableContainer)->getChildByTag(tag));
}

// orig 001c9d60
void DebugSceneLayer::updateView()
{
    EnvTableLayer* settings = getTable(kTagSettingsTable);
    EnvTableLayer* environments = getTable(kTagEnvironmentTable);
    AppConfigData* config = AppConfigData::getInstance();

    settings->clearData();
    settings->addData("env", config->getEnv());
    settings->addData("site_url", config->getSiteUrl());
    settings->addData("resource_url", config->getResourceUrl());
    settings->addData("offset_url", config->getOffsetJsonUrl());
    settings->addData("story_url", config->getStoryJsonUrl());
    settings->addData("story_id", config->getStoryId());
    settings->addData("purchase_post_url", config->getPurchasePostUrl());
    settings->addData("subscribe_post_url", config->getSubscribePostUrl());
    settings->addData(kJumpToDebugPageKey, config->getSiteDebugPageUrl());
    settings->invalidate();

    environments->clearData();
    std::vector<std::string> envList = config->getEnvList();
    for (const std::string& env : envList) environments->addData(env, "");
    environments->invalidate();
}

// orig 001ca2a8
void DebugSceneLayer::switchTable()
{
    EnvTableLayer* settings = getTable(kTagSettingsTable);
    EnvTableLayer* environments = getTable(kTagEnvironmentTable);
    bool settingsShown = settings->isVisible();
    settings->setVisible(!settingsShown);
    settings->setTouchEnabled(!settingsShown);
    environments->setVisible(settingsShown);
    environments->setTouchEnabled(settingsShown);
    updateView();
}

// orig 001ca348
void DebugSceneLayer::onPressListButton(CCObject* /*sender*/, unsigned int /*controlEvent*/)
{
    switchTable();
}

// orig 001ca494
void DebugSceneLayer::onPressCloseButton(CCObject* /*sender*/, unsigned int /*controlEvent*/)
{
    exitScene("");
}

// orig 001ca3a0
void DebugSceneLayer::exitScene(const std::string& url)
{
    std::string sceneName = AvailableSceneNames.toString(AvailableScene_Debug);
    SceneConnector::getInstance()->setSceneName(sceneName.c_str());
    if (!url.empty()) {
        std::string parameter = "{\"url\":\"" + url + "\"}";
        SceneConnector::getInstance()->setCallbackParameter(parameter.c_str());
    }
    CCDirector::sharedDirector()->replaceScene(AfterScene::create());
}

// orig 001c9ba0
SEL_MenuHandler DebugSceneLayer::onResolveCCBCCMenuItemSelector(CCObject* /*target*/, const char* selectorName)
{
    CCLog("onResolveCCBCCMenuItemSelector : %s", selectorName);
    return nullptr;
}

// orig 001c9a58
SEL_CCControlHandler DebugSceneLayer::onResolveCCBCCControlSelector(CCObject* /*target*/,
                                                                     const char* /*selectorName*/)
{
    return nullptr;
}

void DebugSceneLayer::onListMenuItem(CCObject* sender) { onPressListButton(sender, 0); }
void DebugSceneLayer::onCloseMenuItem(CCObject* sender) { onPressCloseButton(sender, 0); }
