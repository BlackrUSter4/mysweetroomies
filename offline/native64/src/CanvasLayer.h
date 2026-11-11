// CanvasLayer: everything drawn behind the text window, as SceneManager listener "canvas_layer". Children (z = tag):
//    0  BackgroundRender
//    1  curtain (black CCLayerColor) for "even" fades on the background
//    2,3 HalfGradientLayer pair for from_center / to_center fades on the background
//    4  three PersonRender slots
//    5  curtain, 6,7 gradient pair - same for fades on the characters
//    8  EffectRender
//    9  curtain, 10,11 gradient pair - same for fades on everything ("both")
//
// Phases:
//   PreRender(prev, cur)   stop curtains and camera, new background, rebuild characters      -> done at once
//   PreEffect(prev, cur)   fade in from cur's fade colour (curtain fades out / gradients open) -> done when finished
//   MainRender(cur)        start character animations, camera pan / screen shake              -> done at once
//   PostEffect(prev)       fade out to prev's fade colour (curtain fades in / gradients close) -> done when finished
#ifndef CANVASLAYER_H
#define CANVASLAYER_H

#include <memory>
#include <vector>

#include "SceneData.h"
#include "SceneEventListener.h"
#include "cocos2d.h"

class BackgroundRender;
class EffectRender;
class PersonRender;

class CanvasLayer : public cocos2d::CCLayer {
public:
    static CanvasLayer *create();
    virtual ~CanvasLayer();
    virtual bool init();
    void initUi();

    // Callbacks handed to SceneManager; each captures `this` and forwards to the matching handle* method.
    SceneEventListener::SetupHandler onSetup();
    SceneEventListener::SceneHandler onMainRender();
    SceneEventListener::SceneHandler onPostEffect();
    SceneEventListener::TransitionHandler onPreRender();
    SceneEventListener::TransitionHandler onPreEffect();
    SceneEventListener::FinishHandler onFinishScene();

    void handleOnSetup(const std::vector<std::shared_ptr<SceneData>> &scenes);
    void handleOnMainRender(std::shared_ptr<SceneData> current);
    void handleOnPostEffect(std::shared_ptr<SceneData> previous);
    void handleOnPreEffect(std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current);
    void handleOnPreRender(std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current);
    void handleOnFinishScene();

    // Action callbacks that report the phase done.
    void onPreRenderFinished(cocos2d::CCObject *sender);
    void onPreEffectFinished(cocos2d::CCObject *sender);
    void onPostEffectFinished(cocos2d::CCObject *sender);
    void onCameraAnimationFinished(cocos2d::CCObject *sender);  // stay where the pan ended
    void onShakeFinished(cocos2d::CCObject *sender);            // back to the current position

    // Camera pan (ViewEffectData::camera) and screen shake (ViewEffectData::shake), run together as action tag 0.
    // `contentSize` is the largest character/canvas size of the scene.
    void onRenderAction(std::shared_ptr<ViewData> view, const cocos2d::CCSize &contentSize);
    void resetAction();  // stops action tag 0 and hides every curtain

    void addCurtain(int tag);                  // hidden black layer, z = tag
    void addDoubleCurtain(int topTag, int bottomTag);  // hidden HalfGradientLayer pair

    cocos2d::CCSize getCanvasSize();  // the window size
    void resetCanvasSize();           // content size = getCanvasSize()

    // Resting position of the canvas; also becomes the current position.
    void setDefaultPosition(const cocos2d::CCPoint &position);
    cocos2d::CCPoint getDefaultPosition();
    void resetPosition();  // current = default, and move there

private:
    BackgroundRender *m_backgroundRender = nullptr;
    std::vector<PersonRender *> m_personRenders;  // one per ViewData::persons index (3)
    EffectRender *m_effectRender = nullptr;
    cocos2d::CCPoint m_defaultPosition;
    cocos2d::CCPoint m_currentPosition;  // where the camera pan left the canvas
};

#endif
