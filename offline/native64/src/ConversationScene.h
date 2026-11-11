// ConversationScene: the story player (AvailableScene_Adventure). Children, back to front:
//   black CCLayerColor, CanvasLayer (background / characters / effects), TextWindowLayer, AppMenuLayer (menu and
//   voice-replay buttons), TouchLayer (tap = next line).
//
// It registers SceneManager listener "conversation_scene", which only gates input: a tap advances the story only
// after MainRender has finished (setTouchEnable(true)) and when the delay flag is set; advancing clears the flag and
// a repeating 0.3 s timer sets it again, so taps cannot skip lines faster than that. onEnterTransitionDidFinish
// prepares SoundEngine and starts the story (offset json + story json from AppConfigData); when the story ends the
// scene is replaced by an AfterScene.
#ifndef CONVERSATIONSCENE_H
#define CONVERSATIONSCENE_H

#include <memory>

#include "SceneData.h"
#include "SceneEventListener.h"
#include "cocos2d.h"

class CanvasLayer;
class TextWindowLayer;

class ConversationScene : public cocos2d::CCScene {
public:
    static ConversationScene *create();
    static ConversationScene *scene();  // same as create()
    virtual ~ConversationScene();
    virtual bool init();
    virtual void onEnterTransitionDidFinish();
    virtual void onExit();  // SoundEngine::release()

    // Callbacks handed to SceneManager; each captures `this` and forwards to the matching handle* method.
    SceneEventListener::SceneHandler onMainRender();
    SceneEventListener::SceneHandler onPostEffect();
    SceneEventListener::TransitionHandler onPreRender();
    SceneEventListener::TransitionHandler onPreEffect();
    SceneEventListener::FinishHandler onFinishScene();

    void handleOnMainRender(std::shared_ptr<SceneData> current);   // touch on
    void handleOnPostEffect(std::shared_ptr<SceneData> previous);  // touch off
    void handleOnPreRender(std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current);  // touch off
    void handleOnPreEffect(std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current);  // touch off
    void handleOnFinishScene();

    // AppMenuLayer buttons.
    void onTouchMenuPopup(cocos2d::CCObject *sender);  // adds an OverlayMenuLayer
    void onTouchVoicePlay(cocos2d::CCObject *sender);  // replays the current voice line

    void setTouchEnable(bool enabled);
    bool isTouchEnable();
    void setDelayEnd();
    bool isDelayEnd();

private:
    void onDelayEndTimer(float dt);  // repeating schedule target; forwards to setDelayEnd()

    TextWindowLayer *m_textWindow = nullptr;
    CanvasLayer *m_canvas = nullptr;
    bool m_touchEnabled = false;
    bool m_delayEnd = false;
};

#endif
