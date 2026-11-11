#include "CCFadeInTo.h"

USING_NS_CC;

// orig 001e00c0
CCFadeInTo::CCFadeInTo() {}

// orig 001e0034
CCFadeInTo::~CCFadeInTo() {}

// orig 001e00e0
CCFadeInTo *CCFadeInTo::create(float duration, int toOpacity)
{
    CCFadeInTo *action = new CCFadeInTo();
    action->initWithDuration(duration, toOpacity);
    action->autorelease();
    return action;
}

// orig 001e0062
bool CCFadeInTo::initWithDuration(float duration, int toOpacity)
{
    if (!CCActionInterval::initWithDuration(duration)) return false;
    m_toOpacity = toOpacity;
    return true;
}

// orig 001e0074
void CCFadeInTo::update(float time)
{
    CCRGBAProtocol *target = dynamic_cast<CCRGBAProtocol *>(m_pTarget);
    if (target != nullptr) target->setOpacity(static_cast<GLubyte>(static_cast<float>(m_toOpacity) * time));
}
