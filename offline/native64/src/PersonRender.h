// PersonRender: one character slot of CanvasLayer (CanvasLayer keeps three, indexed like ViewData::persons). The
// layer has the size of the canvas; each LayerData of the ViewPersonData becomes one child sprite placed by its
// offset. Animated layers (FrameAnimationData) get their layer index as tag so onMainRender can find them again to
// start the blink / lip-sync animation.
#ifndef PERSONRENDER_H
#define PERSONRENDER_H

#include <memory>
#include <vector>

#include "FrameAnimationData.h"
#include "OffsetLayerData.h"
#include "SceneData.h"
#include "cocos2d.h"

class PersonRender : public cocos2d::CCLayer {
public:
    static PersonRender *create();
    virtual ~PersonRender();
    virtual bool init();

    // When `current` has layers: rebuilds the sprites and moves the layer to its slot (centre 0, left -1/4 of the
    // width, right +1/4). `previous` is unused.
    void onPreRender(std::shared_ptr<ViewPersonData> previous, std::shared_ptr<ViewPersonData> current);
    // Starts the frame animations: the mouth ("05_mouth") plays its sequence once, every other animated layer
    // (eyes) loops forever.
    void onMainRender(std::shared_ptr<ViewPersonData> current);
    void resetData();  // removes every sprite

    void addOffsetLayerData(std::shared_ptr<OffsetLayerData> layer, std::shared_ptr<ViewPersonData> person);
    void addFrameAnimationData(std::shared_ptr<FrameAnimationData> layer, std::shared_ptr<ViewPersonData> person,
                               int tag);
    void runFrameAnimation(std::shared_ptr<FrameAnimationData> layer, std::shared_ptr<ViewPersonData> person,
                           int tag);

    // Character images are authored for a 480 px wide canvas: layer width / 480.
    float getCanvasScale();

    // Largest scaled sprite size seen since the last reset (CanvasLayer uses it for the camera pan).
    cocos2d::CCSize getCurrentMaximumContentSize();
    void setCurrentMaximumContentSize(cocos2d::CCSize size);  // replaces it when either dimension is larger
    void resetCurrentMaximumContentSize();                    // back to the window size

private:
    cocos2d::CCSize m_currentMaximumContentSize;
    // GUESS: a vector the original clears in init()/onPreRender() and frees in the destructor but never fills.
    std::vector<cocos2d::CCNode *> m_unusedSprites;
};

#endif
