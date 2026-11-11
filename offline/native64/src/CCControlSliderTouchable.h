#ifndef CCCONTROLSLIDERTOUCHABLE_H
#define CCCONTROLSLIDERTOUCHABLE_H

#include <functional>

#include "GUI/CCControlExtension/CCControlSlider.h"
#include "cocos2d.h"

// CCControlSlider that also reports the end of a drag (OptionsDialog saves the volume then, not on every move).
class CCControlSliderTouchable : public cocos2d::extension::CCControlSlider {
public:
    CCControlSliderTouchable();
    virtual ~CCControlSliderTouchable();

    // Same as CCControlSlider::create(background, progress, thumb) but returns the subclass.
    static CCControlSliderTouchable* create(cocos2d::CCSprite* backgroundSprite, cocos2d::CCSprite* progressSprite,
                                            cocos2d::CCSprite* thumbSprite);

    virtual bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event);
    virtual void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event);  // listener first, then the slider

    void setOnTouchEndListener(std::function<void()> listener);
    void resetOnTouchEndListener();

private:
    std::function<void()> m_onTouchEnd;  // orig +0x15c
};

#endif
