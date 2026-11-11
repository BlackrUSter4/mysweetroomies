// CCFadeInTo: fades a CCRGBAProtocol target from 0 up to a given opacity (cocos2d's CCFadeIn only goes to 255, and
// CCFadeTo starts from the current opacity). OverlayMenuLayer uses it for its dimmed background.
//
//   layer->runAction(CCFadeInTo::create(0.2f, 128));
#ifndef CCFADEINTO_H
#define CCFADEINTO_H

#include "cocos2d.h"

class CCFadeInTo : public cocos2d::CCFadeIn {
public:
    CCFadeInTo();
    virtual ~CCFadeInTo();

    static CCFadeInTo *create(float duration, int toOpacity);
    virtual bool initWithDuration(float duration, int toOpacity);
    // opacity = toOpacity * time
    virtual void update(float time);

private:
    int m_toOpacity = 0;
};

#endif
