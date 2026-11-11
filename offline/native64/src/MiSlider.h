#ifndef MISLIDER_H
#define MISLIDER_H

#include <string>
#include <vector>

#include "cocos2d.h"

// Child tags of the three sprites one alignmentScalableImage() call lays out (left cap, stretched middle, right cap).
struct MiSliderTag {
    int left;
    int middle;
    int right;
};

// Horizontal slider drawn from sprite-frame images: a background bar (tags 0-2), a foreground bar scaled to the
// current value (tags 3-5) and an indicator knob (tag 6). The value is either continuous in
// [indicator min, indicator max] or snaps to the sorted list given to setSliderValues().
// Usage: new + init(), the set* calls, then build(). Changes are broadcast through CCNotificationCenter under a
// per-instance name ("MiSlider<n>") when a touch ends.
class MiSlider : public cocos2d::CCLayer {
public:
    static int GLOBAL_SLIDER_ID;  // last id handed out; init() pre-increments it

    virtual ~MiSlider();
    virtual bool init();

    MiSlider* setFrameCache(const char* plist, const char* textureFile);
    // Image names are sprite-frame names, used as given (the strings must outlive build()).
    MiSlider* setSliderBackground(const char* left, const char* middle, const char* right);
    MiSlider* setSliderForeground(const char* left, const char* middle, const char* right);
    MiSlider* setSliderIndicator(const char* indicator);
    MiSlider* setSliderWidth(float width);
    MiSlider* setIndicatorRange(float minimum, float maximum);  // also moves the position to `minimum`
    MiSlider* setSliderValues(std::vector<float> values);       // sorted; range becomes front()..back()
    std::vector<float> getSliderValues();

    MiSlider* setSliderPosition(float position);  // clamped to the indicator range
    float getSliderPosition();
    MiSlider* setSliderRelativePosition(float relative);  // 0..1 along the bar
    float getSliderRelativePosition();

    // `selector` is called with this slider as argument after each touch that ends on the slider.
    MiSlider* addSliderCallback(cocos2d::CCObject* target, cocos2d::SEL_CallFuncO selector);
    MiSlider* removeSliderCallback(cocos2d::CCObject* target);
    MiSlider* postCallback();

    MiSlider* build();   // creates the sprites and registers the touch delegate
    MiSlider* update();  // re-lays the foreground and the indicator for the current position

    virtual bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event);
    virtual void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event);
    virtual void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event);

protected:
    void alignmentScalableImage(const char** images, MiSliderTag* tags, int zOrder, float middleScale);

private:
    void moveToTouch(cocos2d::CCTouch* touch);

    float m_position = 0.0f;       // orig +0x108
    float m_maximum = 1.0f;        // orig +0x10c
    float m_minimum = 0.0f;        // orig +0x110
    float m_width = 100.0f;        // orig +0x114
    std::vector<float> m_values;   // orig +0x118
    std::string m_notificationName;  // orig +0x124, char[256]
    const char* m_backgroundImages[3] = {nullptr, nullptr, nullptr};  // orig +0x224
    const char* m_foregroundImages[3] = {nullptr, nullptr, nullptr};  // orig +0x230
    const char* m_indicatorImage = nullptr;                           // orig +0x23c
};

#endif
