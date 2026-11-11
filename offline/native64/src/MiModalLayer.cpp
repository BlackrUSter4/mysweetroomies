#include "MiModalLayer.h"

USING_NS_CC;

const int MiModalLayer::kModalLayerPriority;

MiModalLayer::MiModalLayer() : m_closeOnTouch(false) {}

MiModalLayer::~MiModalLayer() {}

// orig 001dc714
bool MiModalLayer::init() {
    if (!CCLayer::init()) return false;
    setTouchPriority(kModalLayerPriority);
    setTouchEnabled(true);
    setTouchMode(kCCTouchesOneByOne);
    m_closeOnTouch = true;
    m_dismissCallback = [] {};
    return true;
}

// orig 001dc648 — swallowing every touch is what makes the layer modal, so this returns true even when the layer
// stays open.
bool MiModalLayer::ccTouchBegan(CCTouch*, CCEvent*) {
    if (m_closeOnTouch) dismiss();
    return true;
}

// orig 001dc61e
void MiModalLayer::dismiss() {
    // Deviation: the original called the std::function unchecked (std::bad_function_call -> abort when a caller
    // passed an empty one to setDismissCallback).
    if (m_dismissCallback) m_dismissCallback();
    removeFromParentAndCleanup(true);
}

void MiModalLayer::setDismissCallback(std::function<void()> callback) {
    m_dismissCallback = std::move(callback);
}

void MiModalLayer::setEnableCloseOnTouchLayer(bool enable) {
    m_closeOnTouch = enable;
}
