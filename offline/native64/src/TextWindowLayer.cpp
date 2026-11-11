#include "TextWindowLayer.h"

#include "SceneManager.h"
#include "TextHelpers.h"

USING_NS_CC;

namespace {

const char *const LISTENER_NAME = "text_window";  // SceneManager listener and auto-play listener name
const char *const ATLAS_PLIST = "Framework/ui/conversation.plist";
const char *const ATLAS_TEXTURE = "Framework/ui/conversation.png";
const char *const FONT = "Application/fonts/07YasashisaGothic.ttf";
const ccColor3B NAME_COLOR = {44, 44, 44};
const ccColor3B BODY_COLOR = {44, 44, 44};

// Frame tables for getTextWindowBackgroundPath: {none/auto, speak, think}.
const std::string MAIN_WINDOW_FRAMES[] = {"serif_box_you.png", "serif_box_say.png", "serif_box_think.png"};
const std::string SUB_WINDOW_FRAMES[] = {"serif_box_front_you.png", "serif_box_front_say.png",
                                         "serif_box_front_think.png"};
const std::string CURSOR_FRAMES[] = {"piko_front.png", "piko_say.png", "piko_think.png"};
const std::string AUTO_PLAY_CURSOR_FRAMES[] = {"auto_play.png", "auto_play.png", "auto_play.png"};

const int SUB_WINDOW_TAG = 1;  // child tag of the layer holding the front window

void setFrame(CCSprite *sprite, const std::string &frameName)
{
    sprite->setDisplayFrame(CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName.c_str()));
}

void reportDone(SceneEventType event)
{
    SceneManager::getInstance()->doneListenerEvent(event, LISTENER_NAME);
}

}  // namespace

TextWindowLayer::TextWindowLayer() {}

// orig 001c8b84
TextWindowLayer *TextWindowLayer::create()
{
    TextWindowLayer *layer = new TextWindowLayer();
    if (!layer->init()) {
        delete layer;
        return nullptr;
    }
    layer->autorelease();
    return layer;
}

// orig 001e32a4
TextWindowLayer::~TextWindowLayer()
{
    SceneManager::getInstance()->removeListener(LISTENER_NAME);
    // Not in the original, which left a dangling `this` in SceneManager's auto-play listeners until the story's
    // listeners were cleared; toggling auto-play after the layer died would crash.
    SceneManager::getInstance()->removeAutoPlayChangedListener(LISTENER_NAME);
}

// orig 001e3774
bool TextWindowLayer::init()
{
    if (!CCLayer::init()) return false;
    initUi();

    auto listener = std::make_shared<SceneEventListener>(
        [](const std::vector<std::shared_ptr<SceneData>> &) { reportDone(SceneEventType_Setup); },  // orig 001e332c
        onMainRender(), onPreRender(), onPreEffect(), onPostEffect(), onFinishScene());
    SceneManager::getInstance()->addListener(LISTENER_NAME, listener);
    // orig 001e3254
    SceneManager::getInstance()->addAutoPlayChangedListener(
        [this](bool enabled, std::shared_ptr<SceneData> current) { onAutoPlayChanged(enabled, current); },
        LISTENER_NAME);

    m_showInSubWindow = false;
    m_previousShowTail = ExSceneShowTail_None;
    return true;
}

// orig 001e2b00
void TextWindowLayer::initUi()
{
    CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    CCSize square(winSize.width, winSize.width);
    CCPoint upper = getTextUpperBound();
    CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile(ATLAS_PLIST, ATLAS_TEXTURE);
    setTextWindowSize(winSize.width, winSize.height - square.height);
    float scale = winSize.width / 640.0f;

    // Pale backdrop of the whole 640x496 design area under the upper bound.
    CCLayerColor *backdrop = CCLayerColor::create(ccc4(204, 255, 255, 255));
    backdrop->setContentSize(CCSize(640.0f, 496.0f));
    backdrop->ignoreAnchorPointForPosition(false);
    backdrop->setAnchorPoint(CCPoint(0.5f, 1.0f));
    backdrop->setPosition(CCPoint(0.0f, upper.y));
    backdrop->setScale(scale);
    addChild(backdrop);

    // --- main window ---
    m_windowBackground = CCSprite::createWithSpriteFrameName(MAIN_WINDOW_FRAMES[0].c_str());
    m_windowBackground->setAnchorPoint(CCPoint(0.5f, 1.0f));
    m_windowBackground->setPosition(CCPoint(0.0f, scale * 12.0f + upper.y));
    m_windowBackground->setScale(scale);
    addChild(m_windowBackground);

    float nameFontSize = scale * 36.0f;
    m_nameLabel = CCLabelTTF::create("", FONT, nameFontSize, CCSize(scale * 596.0f, scale * 52.0f),
                                     kCCTextAlignmentLeft, kCCVerticalTextAlignmentCenter);
    m_nameLabel->setColor(NAME_COLOR);
    m_nameLabel->setAnchorPoint(CCPoint(0.0f, 0.5f));
    float textLeft = scale * -298.0f;
    m_nameLabel->setPosition(CCPoint(textLeft, scale * -62.0f + upper.y));
    addChild(m_nameLabel);

    CCSize windowSize = getTextWindowSize();
    float bodyFontSize = scale * 34.0f;
    // Created with a 0.99-wide unbounded box, then narrowed to 0.9 of the width with the full window height.
    m_bodyLabel = CCLabelTTF::create("", FONT, bodyFontSize, CCSize(windowSize.width * 0.99f, 0.0f),
                                     kCCTextAlignmentLeft, kCCVerticalTextAlignmentTop);
    m_bodyLabel->setDimensions(CCSize(windowSize.width * 0.9f, windowSize.height));
    m_bodyLabel->setColor(BODY_COLOR);
    m_bodyLabel->setAnchorPoint(CCPoint(0.0f, 1.0f));
    m_bodyLabel->setPosition(CCPoint(textLeft, scale * -110.0f + upper.y));
    addChild(m_bodyLabel);

    CCPoint mainCursorPosition = getMainCursorPosition();
    m_mainCursor = CCSprite::createWithSpriteFrameName(CURSOR_FRAMES[0].c_str());
    m_mainCursor->setScale(scale);
    m_mainCursor->setPosition(mainCursorPosition);
    m_mainCursor->runAction(getCursorAction(mainCursorPosition));
    addChild(m_mainCursor);

    // --- front window for linked lines, hidden until needed ---
    CCLayer *subWindow = CCLayer::create();
    m_subWindowBackground = CCSprite::createWithSpriteFrameName(SUB_WINDOW_FRAMES[0].c_str());
    m_subWindowBackground->setAnchorPoint(CCPoint(0.5f, 1.0f));
    m_subWindowBackground->setPosition(CCPoint(0.0f, scale * -68.0f + upper.y));
    m_subWindowBackground->setScale(scale);
    subWindow->addChild(m_subWindowBackground);

    m_subNameLabel = CCLabelTTF::create("", FONT, nameFontSize);
    m_subNameLabel->setColor(NAME_COLOR);
    m_subNameLabel->setAnchorPoint(CCPoint(0.0f, 0.5f));
    float subTextLeft = scale * -280.0f;
    m_subNameLabel->setPosition(CCPoint(subTextLeft, scale * -198.0f + upper.y));
    subWindow->addChild(m_subNameLabel);

    windowSize = getTextWindowSize();
    m_subBodyLabel = CCLabelTTF::create("", FONT, bodyFontSize, CCSize(windowSize.width * 0.99f, 0.0f),
                                        kCCTextAlignmentLeft, kCCVerticalTextAlignmentTop);
    m_subBodyLabel->setDimensions(CCSize(windowSize.width * 0.9f, windowSize.height));
    m_subBodyLabel->setColor(BODY_COLOR);
    m_subBodyLabel->setAnchorPoint(CCPoint(0.0f, 1.0f));
    m_subBodyLabel->setPosition(CCPoint(subTextLeft, scale * -238.0f + upper.y));
    subWindow->addChild(m_subBodyLabel);

    // The front cursor only starts moving once changeSubCursorState runs for the first linked line.
    m_subCursor = CCSprite::createWithSpriteFrameName(CURSOR_FRAMES[0].c_str());
    m_subCursor->setScale(scale);
    m_subCursor->setPosition(getSubCursorPosition());
    subWindow->addChild(m_subCursor);

    subWindow->setVisible(false);
    addChild(subWindow, 1, SUB_WINDOW_TAG);

    // Screens of 16:9 and taller get a strip of artwork under the window, scaled to the screen width.
    if (winSize.height / winSize.width >= 1.7f) {
        CCSprite *footer = CCSprite::createWithSpriteFrameName("footer.png");
        footer->setScale(winSize.width / footer->getContentSize().width);
        footer->setAnchorPoint(CCPoint(0.5f, 0.0f));
        footer->setPosition(CCPoint(0.0f, -winSize.height * 0.5f));
        addChild(footer);
    }
}

// orig 001e2780
SceneEventListener::SceneHandler TextWindowLayer::onMainRender()
{
    return [this](std::shared_ptr<SceneData> current) { handleOnMainRender(current); };  // orig 001e3514
}

// orig 001e27b8
SceneEventListener::SceneHandler TextWindowLayer::onPostEffect()
{
    return [this](std::shared_ptr<SceneData> previous) { handleOnPostEffect(previous); };  // orig 001e357c
}

// orig 001e27f0
SceneEventListener::TransitionHandler TextWindowLayer::onPreRender()
{
    return [this](std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current) {
        handleOnPreRender(previous, current);  // orig 001e36c8
    };
}

// orig 001e2828
SceneEventListener::TransitionHandler TextWindowLayer::onPreEffect()
{
    return [this](std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current) {
        handleOnPreEffect(previous, current);  // orig 001e3628
    };
}

// orig 001e2860
SceneEventListener::FinishHandler TextWindowLayer::onFinishScene()
{
    return [this]() { handleOnFinishScene(); };  // orig 001e3768
}

// orig 001e3344
void TextWindowLayer::handleOnMainRender(std::shared_ptr<SceneData> current)
{
    // Guard not in the original, which dereferenced scene->text unconditionally (the parser always sets it).
    std::shared_ptr<TextData> text = current ? current->text : nullptr;
    if (!text) {
        reportDone(SceneEventType_MainRender);
        return;
    }

    if (!m_showInSubWindow) {
        // A hidden speaker ("__NULL__", narration) clears the name plate; an empty one keeps the previous name.
        if (!text->speaker.empty() || text->speakerHidden) setNameText(text->speaker.c_str());
        if (!text->body.empty()) setBodyText(text->body.c_str());
        if (m_previousShowTail != text->showTail) {
            setFrame(m_windowBackground, getTextWindowBackgroundPath(MAIN_WINDOW_FRAMES, text->showTail));
            changeMainCursorState(SceneManager::getInstance()->isAutoPlay(), text);
        }
        getChildByTag(SUB_WINDOW_TAG)->setVisible(false);
    } else {
        if (!text->speaker.empty()) setSubNameText(text->speaker.c_str());
        if (!text->body.empty()) setSubBodyText(text->body.c_str());
        setFrame(m_subWindowBackground, getTextWindowBackgroundPath(SUB_WINDOW_FRAMES, text->showTail));
        changeSubCursorState(SceneManager::getInstance()->isAutoPlay(), text);
        getChildByTag(SUB_WINDOW_TAG)->setVisible(true);
    }
    reportDone(SceneEventType_MainRender);
}

// orig 001e3564
void TextWindowLayer::handleOnPostEffect(std::shared_ptr<SceneData>)
{
    reportDone(SceneEventType_PostEffect);
}

// orig 001e35cc
void TextWindowLayer::handleOnPreEffect(std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData>)
{
    // Decided from the scene being left: if it linked into this one, this line goes into the front window while
    // the previous line stays visible behind it.
    if (previous && previous->text) {
        m_showInSubWindow = previous->text->linkScene == ExSceneLinkScene_Link;
        m_previousShowTail = previous->text->showTail;
    } else {
        m_previousShowTail = ExSceneShowTail_None;
        m_showInSubWindow = false;
    }
    reportDone(SceneEventType_PreEffect);
}

// orig 001e36b0
void TextWindowLayer::handleOnPreRender(std::shared_ptr<SceneData>, std::shared_ptr<SceneData>)
{
    reportDone(SceneEventType_PreRender);
}

// orig 001e3750
void TextWindowLayer::handleOnFinishScene()
{
    reportDone(SceneEventType_Finish);
}

// orig 001e31d8
void TextWindowLayer::onAutoPlayChanged(bool enabled, std::shared_ptr<SceneData> current)
{
    if (!current || !current->text) return;
    changeMainCursorState(enabled, current->text);
    changeSubCursorState(enabled, current->text);
}

// orig 001e2898
CCSize TextWindowLayer::getTextWindowSize()
{
    return m_textWindowSize;
}

// orig 001e2ae8
void TextWindowLayer::setTextWindowSize(float width, float height)
{
    m_textWindowSize = CCSize(width, height);
}

// orig 001e28a8
CCPoint TextWindowLayer::getMainCursorPosition()
{
    float scale = getSpriteScale();
    CCPoint upper = getTextUpperBound();
    CCSize windowSize = getTextWindowSize();
    return CCPoint(windowSize.width * 0.5f + scale * -65.0f, scale * -240.0f + upper.y);
}

// orig 001e29c8
CCPoint TextWindowLayer::getSubCursorPosition()
{
    float scale = getSpriteScale();
    CCPoint upper = getTextUpperBound();
    CCSize windowSize = getTextWindowSize();
    return CCPoint(windowSize.width * 0.5f + scale * -55.0f, scale * -268.0f + upper.y);
}

// orig 001e290c
void TextWindowLayer::changeMainCursorState(bool autoPlay, std::shared_ptr<TextData> text)
{
    m_mainCursor->stopAllActions();
    CCPoint position = getMainCursorPosition();
    if (!autoPlay) {
        setFrame(m_mainCursor, getTextWindowBackgroundPath(CURSOR_FRAMES, text->showTail));
        m_mainCursor->runAction(getCursorAction(position));
    } else {
        setFrame(m_mainCursor, getTextWindowBackgroundPath(AUTO_PLAY_CURSOR_FRAMES, text->showTail));
        m_mainCursor->runAction(getAutoPlayCursorAction());
    }
}

// orig 001e2a2c
void TextWindowLayer::changeSubCursorState(bool autoPlay, std::shared_ptr<TextData> text)
{
    m_subCursor->stopAllActions();
    CCPoint position = getSubCursorPosition();
    if (!autoPlay) {
        setFrame(m_subCursor, getTextWindowBackgroundPath(CURSOR_FRAMES, text->showTail));
        m_subCursor->runAction(getCursorAction(position));
    } else {
        setFrame(m_subCursor, getTextWindowBackgroundPath(AUTO_PLAY_CURSOR_FRAMES, text->showTail));
        m_subCursor->runAction(getAutoPlayCursorAction());
    }
}

// orig 001e3148 .. 001e31c6
void TextWindowLayer::setNameText(const char *text) { m_nameLabel->setString(text); }
const char *TextWindowLayer::getNameText() { return m_nameLabel->getString(); }
void TextWindowLayer::setBodyText(const char *text) { m_bodyLabel->setString(text); }
const char *TextWindowLayer::getBodyText() { return m_bodyLabel->getString(); }
void TextWindowLayer::setSubNameText(const char *text) { m_subNameLabel->setString(text); }
const char *TextWindowLayer::getSubNameText() { return m_subNameLabel->getString(); }
void TextWindowLayer::setSubBodyText(const char *text) { m_subBodyLabel->setString(text); }
const char *TextWindowLayer::getSubBodyText() { return m_subBodyLabel->getString(); }
