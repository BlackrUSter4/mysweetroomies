#include "CCControlSliderTouchable.h"

USING_NS_CC;
USING_NS_CC_EXT;

// orig 001dc1d0
CCControlSliderTouchable::CCControlSliderTouchable() : m_onTouchEnd([] {}) {}

CCControlSliderTouchable::~CCControlSliderTouchable() {}

// orig 001dc24c — like the original, the result of initWithSprites is not checked.
CCControlSliderTouchable* CCControlSliderTouchable::create(CCSprite* backgroundSprite, CCSprite* progressSprite,
                                                           CCSprite* thumbSprite) {
    CCControlSliderTouchable* slider = new CCControlSliderTouchable();
    slider->initWithSprites(backgroundSprite, progressSprite, thumbSprite);
    slider->autorelease();
    return slider;
}

bool CCControlSliderTouchable::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    return CCControlSlider::ccTouchBegan(touch, event);
}

// orig 001dc494
void CCControlSliderTouchable::ccTouchEnded(CCTouch* touch, CCEvent* event) {
    // Deviation: the original called the std::function unchecked (abort if a caller set an empty one).
    if (m_onTouchEnd) m_onTouchEnd();
    CCControlSlider::ccTouchEnded(touch, event);
}

void CCControlSliderTouchable::setOnTouchEndListener(std::function<void()> listener) {
    m_onTouchEnd = std::move(listener);
}

void CCControlSliderTouchable::resetOnTouchEndListener() {
    m_onTouchEnd = [] {};
}
