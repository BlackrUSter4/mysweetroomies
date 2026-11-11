#include "ConversationScene.h"

#include "AfterScene.h"
#include "AppConfigData.h"
#include "AppMenuLayer.h"
#include "CanvasLayer.h"
#include "OverlayMenuLayer.h"
#include "SceneManager.h"
#include "SoundEngine.h"
#include "TextLogManager.h"
#include "TextWindowLayer.h"
#include "TouchLayer.h"

USING_NS_CC;

namespace {
const char *const kListenerName = "conversation_scene";
const float kTapInterval = 0.3f;  // seconds between two taps that may advance the story

void reportDone(SceneEventType event)
{
    SceneManager::getInstance()->doneListenerEvent(event, kListenerName);
}
}  // namespace

// orig 001c8474
ConversationScene *ConversationScene::create()
{
    ConversationScene *scene = new ConversationScene();
    if (!scene->init()) {
        delete scene;
        return nullptr;
    }
    scene->autorelease();
    return scene;
}

// orig 001c8ddc
ConversationScene *ConversationScene::scene()
{
    return create();
}

// orig 001c9160
ConversationScene::~ConversationScene()
{
    SceneManager::getInstance()->removeListener(kListenerName);
}

// orig 001c9510
bool ConversationScene::init()
{
    // Like the original, CCScene::init() is not called (it would only set the content size, which nothing reads).
    const CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    m_touchEnabled = true;
    m_delayEnd = false;

    addChild(CCLayerColor::create(ccc4(0, 0, 0, 255)));

    m_canvas = CanvasLayer::create();
    const CCSize canvasSize = m_canvas->getCanvasSize();
    m_canvas->setDefaultPosition(CCPoint(winSize.width * 0.5f, winSize.height - canvasSize.height * 0.5f));
    addChild(m_canvas);

    // The text window's origin sits at the screen centre (TextWindowLayer lays itself out around it).
    m_textWindow = TextWindowLayer::create();
    m_textWindow->ignoreAnchorPointForPosition(false);
    m_textWindow->setAnchorPoint(CCPoint(0.0f, 0.0f));
    m_textWindow->setPosition(CCPoint(winSize.width * 0.5f, winSize.height * 0.5f));
    m_textWindow->setNameText("");
    m_textWindow->setBodyText("");
    addChild(m_textWindow);

    AppMenuLayer *menu = AppMenuLayer::create();
    menu->setAppSetting(menu_selector(ConversationScene::onTouchVoicePlay),
                        menu_selector(ConversationScene::onTouchMenuPopup));
    addChild(menu);

    // Repeats for the scene's lifetime; every tick re-arms the tap that advances the story.
    if (!isDelayEnd()) schedule(schedule_selector(ConversationScene::onDelayEndTimer), kTapInterval);

    TouchLayer *touch = TouchLayer::create();
    touch->setTouchCallback(
        [this]() {
            if (isTouchEnable() && isDelayEnd()) {
                SceneManager::getInstance()->next();
                m_delayEnd = false;
            }
        },
        1);
    addChild(touch, 0, 0);

    SceneManager::getInstance()->addListener(
        kListenerName,
        std::make_shared<SceneEventListener>(
            [](const std::vector<std::shared_ptr<SceneData>> &) { reportDone(SceneEventType_Setup); },
            onMainRender(), onPreRender(), onPreEffect(), onPostEffect(), onFinishScene()));

    SceneManager::getInstance()->setOnStoryFinishListener(
        []() { CCDirector::sharedDirector()->replaceScene(AfterScene::create()); });

    // The backlog lives in memory only: it starts empty for every story and is never written anywhere.
    TextLogManager::getInstance()->prepare([]() -> const std::string { return "[]"; },
                                           [](const std::string &) {});
    return true;
}

// orig 001c91a0
void ConversationScene::onEnterTransitionDidFinish()
{
    CCScene::onEnterTransitionDidFinish();
    SoundEngine::getInstance()->prepare(
        [](const std::string &key, float defaultValue) -> const float {
            return static_cast<float>(
                AppConfigData::getInstance()->getDouble(key, static_cast<double>(defaultValue)));
        },
        [](const std::string &key, float value) {
            AppConfigData::getInstance()->putDouble(key, static_cast<double>(value)).commit();
        });
    SceneManager::getInstance()->start(AppConfigData::getInstance()->getOffsetJsonFilePath(),
                                       AppConfigData::getInstance()->getStoryJsonFilePath());
}

// orig 001c8a5c
void ConversationScene::onExit()
{
    CCScene::onExit();
    SoundEngine::getInstance()->release();
}

// orig 001c8e00
SceneEventListener::SceneHandler ConversationScene::onMainRender()
{
    return [this](std::shared_ptr<SceneData> current) { handleOnMainRender(current); };
}

// orig 001c8e38
SceneEventListener::SceneHandler ConversationScene::onPostEffect()
{
    return [this](std::shared_ptr<SceneData> previous) { handleOnPostEffect(previous); };
}

// orig 001c8ea8
SceneEventListener::TransitionHandler ConversationScene::onPreRender()
{
    return [this](std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current) {
        handleOnPreRender(previous, current);
    };
}

// orig 001c8e70
SceneEventListener::TransitionHandler ConversationScene::onPreEffect()
{
    return [this](std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current) {
        handleOnPreEffect(previous, current);
    };
}

// orig 001c8ee0
SceneEventListener::FinishHandler ConversationScene::onFinishScene()
{
    return [this]() { handleOnFinishScene(); };
}

// orig 001c92cc
void ConversationScene::handleOnMainRender(std::shared_ptr<SceneData> current)
{
    setTouchEnable(true);
    reportDone(SceneEventType_MainRender);
}

// orig 001c9338
void ConversationScene::handleOnPostEffect(std::shared_ptr<SceneData> previous)
{
    setTouchEnable(false);
    reportDone(SceneEventType_PostEffect);
}

// orig 001c93a4
void ConversationScene::handleOnPreRender(std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current)
{
    setTouchEnable(false);
    reportDone(SceneEventType_PreRender);
}

// orig 001c9448
void ConversationScene::handleOnPreEffect(std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current)
{
    setTouchEnable(false);
    reportDone(SceneEventType_PreEffect);
}

// orig 001c94ec
void ConversationScene::handleOnFinishScene()
{
    reportDone(SceneEventType_Finish);
}

// orig 001c8b6e
void ConversationScene::onTouchMenuPopup(CCObject *sender)
{
    // AppMenuLayer makes itself the menu item's target (as the original did), so at run time `this` is the
    // AppMenuLayer, not this scene. Touch nothing here but the CCNode interface: the overlay lands in the menu layer.
    addChild(OverlayMenuLayer::create());
}

// orig 001c8a6c
void ConversationScene::onTouchVoicePlay(CCObject *sender)
{
    SoundEngine::getInstance()->playVoice();
}

// orig 001c8f18
void ConversationScene::setTouchEnable(bool enabled)
{
    m_touchEnabled = enabled;
}

// orig 001c8f1e
bool ConversationScene::isTouchEnable()
{
    return m_touchEnabled;
}

// orig 001c8594
void ConversationScene::setDelayEnd()
{
    m_delayEnd = true;
}

// orig 001c8f24
bool ConversationScene::isDelayEnd()
{
    return m_delayEnd;
}

// The original scheduled setDelayEnd() itself, cast to a SEL_SCHEDULE.
void ConversationScene::onDelayEndTimer(float dt)
{
    setDelayEnd();
}
