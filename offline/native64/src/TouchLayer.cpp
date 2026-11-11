#include "TouchLayer.h"

USING_NS_CC;

TouchLayer* TouchLayer::create() {
    TouchLayer* layer = new TouchLayer();
    if (layer->init()) {
        layer->autorelease();
        return layer;
    }
    delete layer;
    return nullptr;
}

TouchLayer::~TouchLayer() {}

// orig 001e3bcc
bool TouchLayer::init() {
    if (!CCLayerColor::initWithColor(ccc4(0xff, 0xff, 0xff, 0x00))) return false;
    m_touchCallback = [] {};
    m_handlingTouches = false;
    m_touchPriority = 1;
    return true;
}

// orig 001e3a8a
bool TouchLayer::ccTouchBegan(CCTouch*, CCEvent*) {
    // Deviation: the original called the std::function unchecked (abort if a caller set an empty one).
    if (m_touchCallback) m_touchCallback();
    return true;
}

// orig 001e3c64
void TouchLayer::setTouchCallback(std::function<void()> callback, int priority) {
    m_touchCallback = std::move(callback);
    // Kept from the original: this also registers the layer as a standard (all-at-once) delegate through CCLayer;
    // that registration only feeds the unused ccTouches* handlers.
    setTouchEnabled(true);
    m_touchPriority = priority;
    setTouchLayerEventHandle(true);
}

// orig 001e3b84
void TouchLayer::setTouchLayerEventHandle(bool enable) {
    if (m_handlingTouches == enable) return;
    CCTouchDispatcher* dispatcher = CCDirector::sharedDirector()->getTouchDispatcher();
    if (enable)
        dispatcher->addTargetedDelegate(this, m_touchPriority, true);
    else
        dispatcher->removeDelegate(this);
    m_handlingTouches = enable;
}
