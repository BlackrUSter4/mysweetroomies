// TextWindowLayer: the conversation text box at the bottom of ConversationScene - name plate, body text, window
// frame (narration / speech / thought style) and the "tap to continue" cursor, plus a second "front" window that
// is shown on top when the previous line linked straight into this one (ExSceneLinkScene_Link).
//
// It registers itself with SceneManager as listener "text_window" and as an auto-play listener of the same name:
//   PreEffect(prev, cur)  remembers whether `prev` linked into this scene and prev's window style
//   MainRender(cur)       fills the main window, or the front window when linked; swaps frames/cursors
//   auto-play changed     cursor turns into the blinking auto-play mark and back
// Every other phase is reported done immediately. Lengths are in 640-wide design pixels times getSpriteScale();
// the layer is meant to sit with its origin at the screen centre (see getTextUpperBound()).
#ifndef TEXTWINDOWLAYER_H
#define TEXTWINDOWLAYER_H

#include <memory>

#include "ExEnums.h"
#include "SceneData.h"
#include "SceneEventListener.h"
#include "TextData.h"
#include "cocos2d.h"

class TextWindowLayer : public cocos2d::CCLayer {
public:
    static TextWindowLayer *create();
    virtual ~TextWindowLayer();
    virtual bool init();
    void initUi();

    // Callbacks handed to SceneManager; each captures `this` and forwards to the matching handle* method.
    SceneEventListener::SceneHandler onMainRender();
    SceneEventListener::SceneHandler onPostEffect();
    SceneEventListener::TransitionHandler onPreRender();
    SceneEventListener::TransitionHandler onPreEffect();
    SceneEventListener::FinishHandler onFinishScene();

    void handleOnMainRender(std::shared_ptr<SceneData> current);
    void handleOnPostEffect(std::shared_ptr<SceneData> previous);
    void handleOnPreEffect(std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current);
    void handleOnPreRender(std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current);
    void handleOnFinishScene();
    void onAutoPlayChanged(bool enabled, std::shared_ptr<SceneData> current);

    // Area below the upper bound: (winWidth, winHeight - winWidth), set by initUi.
    cocos2d::CCSize getTextWindowSize();
    void setTextWindowSize(float width, float height);

    // Resting points of the two cursors (bottom right of each window).
    cocos2d::CCPoint getMainCursorPosition();
    cocos2d::CCPoint getSubCursorPosition();
    // autoPlay: blinking auto-play mark; otherwise the bouncing cursor of the text's window style.
    void changeMainCursorState(bool autoPlay, std::shared_ptr<TextData> text);
    void changeSubCursorState(bool autoPlay, std::shared_ptr<TextData> text);

    // Main window labels ...
    void setNameText(const char *text);
    const char *getNameText();
    void setBodyText(const char *text);
    const char *getBodyText();
    // ... and the front ("sub") window shown for linked lines.
    void setSubNameText(const char *text);
    const char *getSubNameText();
    void setSubBodyText(const char *text);
    const char *getSubBodyText();

private:
    TextWindowLayer();

    bool m_showInSubWindow = false;                         // previous scene linked into the current one
    ExSceneShowTail m_previousShowTail = ExSceneShowTail_None;  // window style of the previous scene
    cocos2d::CCSize m_textWindowSize;
    cocos2d::CCLabelTTF *m_nameLabel = nullptr;
    cocos2d::CCLabelTTF *m_bodyLabel = nullptr;
    cocos2d::CCLabelTTF *m_subNameLabel = nullptr;
    cocos2d::CCLabelTTF *m_subBodyLabel = nullptr;
    cocos2d::CCSprite *m_windowBackground = nullptr;
    cocos2d::CCSprite *m_subWindowBackground = nullptr;
    cocos2d::CCSprite *m_mainCursor = nullptr;
    cocos2d::CCSprite *m_subCursor = nullptr;
};

#endif
