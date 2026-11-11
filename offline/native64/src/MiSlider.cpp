#include "MiSlider.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

USING_NS_CC;

int MiSlider::GLOBAL_SLIDER_ID = 0;

namespace {

const MiSliderTag kBackgroundTags = {0, 1, 2};
const MiSliderTag kForegroundTags = {3, 4, 5};
const int kTagIndicator = 6;

const int kZBackground = 1;
const int kZForeground = 2;
const int kZIndicator = 3;

// Just above MiModalLayer::kModalLayerPriority (-130), so a slider inside a modal dialog still gets its touches.
const int kSliderTouchPriority = -129;

}  // namespace

MiSlider::~MiSlider() {}

// orig 001dd4ec
bool MiSlider::init() {
    if (!CCLayer::init()) return false;

    m_position = 0.0f;
    m_minimum = 0.0f;
    m_maximum = 1.0f;
    m_width = 100.0f;

    char name[80];
    snprintf(name, sizeof(name), "MiSlider%d", ++GLOBAL_SLIDER_ID);
    m_notificationName = name;

    // The bar ends reuse the "left" image on both sides.
    m_backgroundImages[0] = "slidebar_back_left.png";
    m_backgroundImages[1] = "slidebar_back_middle.png";
    m_backgroundImages[2] = "slidebar_back_left.png";
    m_foregroundImages[0] = "slidebar_front_left.png";
    m_foregroundImages[1] = "slidebar_front_middle.png";
    m_foregroundImages[2] = "slidebar_front_left.png";
    m_indicatorImage = "slidebar_indicator.png";
    return true;
}

MiSlider* MiSlider::setFrameCache(const char* plist, const char* textureFile) {
    CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile(plist, textureFile);
    return this;
}

MiSlider* MiSlider::setSliderBackground(const char* left, const char* middle, const char* right) {
    m_backgroundImages[0] = left;
    m_backgroundImages[1] = middle;
    m_backgroundImages[2] = right;
    return this;
}

MiSlider* MiSlider::setSliderForeground(const char* left, const char* middle, const char* right) {
    m_foregroundImages[0] = left;
    m_foregroundImages[1] = middle;
    m_foregroundImages[2] = right;
    return this;
}

MiSlider* MiSlider::setSliderIndicator(const char* indicator) {
    m_indicatorImage = indicator;
    return this;
}

MiSlider* MiSlider::setSliderWidth(float width) {
    m_width = width;
    return this;
}

MiSlider* MiSlider::setIndicatorRange(float minimum, float maximum) {
    m_maximum = maximum;
    m_minimum = minimum;
    m_position = minimum;
    return this;
}

// orig 001dd458
MiSlider* MiSlider::setSliderValues(std::vector<float> values) {
    m_values = std::move(values);
    std::sort(m_values.begin(), m_values.end());
    // Deviation: the original read values.at(0) unconditionally (std::out_of_range -> abort for an empty list).
    if (!m_values.empty()) setIndicatorRange(m_values.front(), m_values.back());
    return this;
}

std::vector<float> MiSlider::getSliderValues() {
    return m_values;
}

// orig 001dcc0a
MiSlider* MiSlider::setSliderPosition(float position) {
    if (position > m_maximum) position = m_maximum;
    if (position < m_minimum) position = m_minimum;
    m_position = position;
    return this;
}

float MiSlider::getSliderPosition() {
    return m_position;
}

// orig 001dcc3c
MiSlider* MiSlider::setSliderRelativePosition(float relative) {
    if (relative > 1.0f)
        relative = 1.0f;
    else if (relative < 0.0f)
        relative = 0.0f;

    const size_t count = m_values.size();
    if (count < 2) {
        setSliderPosition((m_maximum - m_minimum) * relative + m_minimum);
        return this;
    }

    // Snap to the first value whose evenly spaced slot lies within 1/count of the touch; none matching means the
    // first value.
    const float tolerance = 1.0f / static_cast<float>(count);
    size_t snapped = 0;
    for (size_t i = 0; i < count; ++i) {
        float slot = static_cast<float>(i) / static_cast<float>(count - 1);
        if (std::fabs(relative - slot) <= tolerance) {
            snapped = i;
            break;
        }
    }
    setSliderPosition(m_values.at(snapped));
    return this;
}

// orig 001dcd3c — note the asymmetry kept from the original: two values already count as "continuous" here,
// while setSliderRelativePosition snaps from two values on.
float MiSlider::getSliderRelativePosition() {
    const size_t count = m_values.size();
    if (count < 3) return (m_position - m_minimum) / (m_maximum - m_minimum);

    size_t index = 0;
    for (size_t i = 0; i < count; ++i) {
        if (m_values[i] == m_position) {
            index = i;
            break;
        }
    }
    return static_cast<float>(index) / static_cast<float>(count - 1);
}

// orig 001dca64
void MiSlider::alignmentScalableImage(const char** images, MiSliderTag* tags, int zOrder, float middleScale) {
    CCSprite* left = CCSprite::createWithSpriteFrameName(images[0]);
    CCSprite* middle = CCSprite::createWithSpriteFrameName(images[1]);
    // Kept from the original: the right cap is built from the middle image; images[2] is never used.
    CCSprite* right = CCSprite::createWithSpriteFrameName(images[1]);

    left->setAnchorPoint(ccp(0.0f, 0.5f));
    middle->setAnchorPoint(ccp(0.0f, 0.5f));
    right->setAnchorPoint(ccp(1.0f, 0.5f));

    left->setPosition(ccp(0.0f, 0.0f));
    middle->setScaleX(middleScale * m_width);
    middle->setPosition(ccp(left->getContentSize().width, 0.0f));
    right->setPosition(ccp(middle->getPosition().x + middle->getContentSize().width * 0.5f, 0.0f));

    addChild(left, zOrder, tags->left);
    addChild(middle, zOrder, tags->middle);
    addChild(right, zOrder, tags->right);
}

MiSlider* MiSlider::addSliderCallback(CCObject* target, SEL_CallFuncO selector) {
    CCNotificationCenter::sharedNotificationCenter()->addObserver(target, selector, m_notificationName.c_str(), this);
    return this;
}

MiSlider* MiSlider::removeSliderCallback(CCObject* target) {
    CCNotificationCenter::sharedNotificationCenter()->removeObserver(target, m_notificationName.c_str());
    return this;
}

MiSlider* MiSlider::postCallback() {
    CCNotificationCenter::sharedNotificationCenter()->postNotification(m_notificationName.c_str());
    return this;
}

// orig 001dcdbc
MiSlider* MiSlider::build() {
    float relative = getSliderRelativePosition();
    MiSliderTag backgroundTags = kBackgroundTags;
    MiSliderTag foregroundTags = kForegroundTags;
    alignmentScalableImage(m_backgroundImages, &backgroundTags, kZBackground, 1.0f);
    alignmentScalableImage(m_foregroundImages, &foregroundTags, kZForeground, relative);

    CCSprite* indicator = CCSprite::createWithSpriteFrameName(m_indicatorImage);
    indicator->setAnchorPoint(ccp(0.5f, 0.5f));
    indicator->setPosition(ccp(relative * m_width, 0.0f));
    setContentSize(CCSize(m_width, indicator->getContentSize().height));
    addChild(indicator, kZIndicator, kTagIndicator);

    // Registered directly rather than through setTouchEnabled, as the original did; it is never removed again.
    CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, kSliderTouchPriority, true);
    return this;
}

// orig 001dcea6
MiSlider* MiSlider::update() {
    CCSize size = getContentSize();
    float relative = getSliderRelativePosition();

    CCNode* foregroundMiddle = getChildByTag(kForegroundTags.middle);
    foregroundMiddle->setScaleX(relative * size.width);
    CCNode* foregroundRight = getChildByTag(kForegroundTags.right);
    foregroundRight->setPosition(
        ccp(foregroundMiddle->getPosition().x + foregroundMiddle->getContentSize().width * 0.5f, 0.0f));
    CCNode* indicator = getChildByTag(kTagIndicator);
    indicator->setPosition(ccp(relative * size.width, 0.0f));
    return this;
}

// The bar is laid out from the slider's own position, and touches are compared in world coordinates; the
// original assumed the slider's parent sits at the origin.
void MiSlider::moveToTouch(CCTouch* touch) {
    CCPoint location = touch->getLocation();
    setSliderRelativePosition((location.x - getPositionX()) / getContentSize().width);
    update();
}

// orig 001dcf68
bool MiSlider::ccTouchBegan(CCTouch* touch, CCEvent*) {
    CCSize size = getContentSize();
    CCRect hitArea(getPositionX(), getPositionY() - size.height * 0.5f, size.width, size.height);
    CCPoint location = touch->getLocation();
    if (hitArea.getMinX() <= location.x && location.x <= hitArea.getMaxX() && hitArea.getMinY() <= location.y &&
        location.y <= hitArea.getMaxY()) {
        moveToTouch(touch);
        return true;
    }
    return false;
}

void MiSlider::ccTouchMoved(CCTouch* touch, CCEvent*) {
    moveToTouch(touch);
}

void MiSlider::ccTouchEnded(CCTouch* touch, CCEvent*) {
    moveToTouch(touch);
    postCallback();
}
