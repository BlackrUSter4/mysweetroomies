#ifndef TOUCHLAYER_H
#define TOUCHLAYER_H

#include <functional>

#include "cocos2d.h"

// Invisible (white, alpha 0) colour layer that calls a callback on every touch it receives and swallows it.
// ConversationScene uses one as "tap to advance".
class TouchLayer : public cocos2d::CCLayerColor {
public:
    static TouchLayer* create();
    virtual ~TouchLayer();
    virtual bool init();

    virtual bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event);

    // Installs `callback` and registers as a targeted delegate at `priority`.
    void setTouchCallback(std::function<void()> callback, int priority);
    // Adds/removes the targeted delegate; no-op when already in the requested state.
    void setTouchLayerEventHandle(bool enable);

private:
    std::function<void()> m_touchCallback;  // orig +0x184
    bool m_handlingTouches = false;         // orig +0x194
    int m_touchPriority = 1;                // orig +0x198
};

#endif
