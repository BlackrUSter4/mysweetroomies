#include "TextHelpers.h"

USING_NS_CC;

// orig 001e2730
float getSpriteScale()
{
    return CCDirector::sharedDirector()->getWinSize().width / 640.0f;
}

// orig 001e2674
CCPoint getTextUpperBound()
{
    CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    // The original builds a square CCSize(width, width) and subtracts its height: the text area is the bottom
    // winWidth pixels of the screen, measured from the centre.
    CCSize square(winSize.width, winSize.width);
    return CCPoint(0.0f, winSize.height * 0.5f - square.height);
}

// orig 001e26c0
CCAction *getCursorAction(CCPoint &position)
{
    const float halfPeriod = 0.6f;
    CCFiniteTimeAction *bounce =
        CCSequence::create(CCMoveTo::create(halfPeriod, CCPoint(position.x, position.y + 10.0f)),
                           CCMoveTo::create(halfPeriod, CCPoint(position.x, position.y)), NULL);
    // The instant fade undoes a half-finished auto-play blink when the cursor switches back.
    return CCSequence::create(CCFadeTo::create(0.0f, 255), CCRepeat::create(bounce, 0x7fffffff), NULL);
}

// orig 001e2648
CCAction *getAutoPlayCursorAction()
{
    CCFiniteTimeAction *blink = CCSequence::create(CCFadeOut::create(0.75f), CCFadeIn::create(0.75f), NULL);
    return CCRepeat::create(blink, 0x7fffffff);
}

// orig 001e274c
std::string getTextWindowBackgroundPath(const std::string *paths, ExSceneShowTail showTail)
{
    if (showTail == ExSceneShowTail_Speak) return paths[1];
    if (showTail == ExSceneShowTail_Think) return paths[2];
    return paths[0];
}
