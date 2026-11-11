#include "EnvTableLayer.h"

USING_NS_CC;
USING_NS_CC_EXT;

namespace {

const int kTagTableView = 0;
const int kTagKeyLabel = 0;
const int kTagValueLabel = 1;

// Cell layout in design pixels (640-wide screen), scaled by winSize.width / 640.
const float kDesignWidth = 640.0f;
const float kCellHeight = 140.0f;
const float kTextLeft = 20.0f;
const float kTextWidth = 600.0f;
const float kKeyFontSize = 32.0f;
const float kKeyTop = 100.0f;
const float kKeyHeight = 42.0f;
const float kValueFontSize = 22.0f;
const float kValueTop = 80.0f;
const float kValueHeight = 76.0f;
const float kSeparatorY = 5.0f;
const ccColor4B kSeparatorColor = {0x34, 0x34, 0x34, 0xff};

float designScale() {
    return CCDirector::sharedDirector()->getWinSize().width / kDesignWidth;
}

}  // namespace

EnvTableLayer* EnvTableLayer::create(const CCSize& size) {
    EnvTableLayer* layer = new EnvTableLayer();
    if (layer->initWithSize(size)) {
        layer->autorelease();
        return layer;
    }
    delete layer;
    return nullptr;
}

EnvTableLayer::~EnvTableLayer() {}

// orig 001c7c14
bool EnvTableLayer::initWithSize(const CCSize& size) {
    if (!CCLayer::init()) return false;
    setContentSize(size);

    CCTableView* table = CCTableView::create(this, getContentSize());
    table->setVerticalFillOrder(kCCTableViewFillTopDown);
    table->setDelegate(this);
    table->setTag(kTagTableView);
    addChild(table);

    m_callback = [](const std::string&, const std::string&) {};
    return true;
}

// orig 001c75ec
void EnvTableLayer::setTouchEnabled(bool enabled) {
    CCLayer::setTouchEnabled(enabled);
    // Deviation: null-checked; the original dereferenced the child unconditionally (crash if called on a layer
    // whose initWithSize failed or before it ran).
    if (CCLayer* table = static_cast<CCLayer*>(getChildByTag(kTagTableView))) table->setTouchEnabled(enabled);
}

void EnvTableLayer::setCallback(std::function<void(const std::string&, const std::string&)> callback) {
    m_callback = std::move(callback);
}

// orig 001c7ff0
void EnvTableLayer::setData(std::map<const std::string, const std::string> data) {
    m_rows.clear();
    for (const auto& entry : data) m_rows.emplace_back(entry.first, entry.second);
}

void EnvTableLayer::addData(const std::string& key, std::string value) {
    m_rows.emplace_back(key, value);
}

void EnvTableLayer::clearData() {
    m_rows.clear();
}

// orig 001c7b98 — the original also copied every row into a temporary and dropped it again (no effect).
void EnvTableLayer::invalidate() {
    CCTableView* table = static_cast<CCTableView*>(getChildByTag(kTagTableView));
    if (table) table->reloadData();
}

// orig 001c7624 — square cells in the original; CCTableView only uses the height for a vertical table.
CCSize EnvTableLayer::tableCellSizeForIndex(CCTableView*, unsigned int) {
    float side = designScale() * kCellHeight;
    return CCSize(side, side);
}

// orig 001c782c
CCTableViewCell* EnvTableLayer::tableCellAtIndex(CCTableView* table, unsigned int idx) {
    const EnvTableLayerData row = m_rows.at(idx);

    CCTableViewCell* cell = table->dequeueCell();
    if (cell) {
        static_cast<CCLabelTTF*>(cell->getChildByTag(kTagKeyLabel))->setString(row.key.c_str());
        static_cast<CCLabelTTF*>(cell->getChildByTag(kTagValueLabel))->setString(row.value.c_str());
        return cell;
    }

    cell = new CCTableViewCell();
    cell->autorelease();
    const float scale = designScale();
    cell->setContentSize(CCSize(scale * kDesignWidth, scale * kCellHeight));

    CCLabelTTF* keyLabel = CCLabelTTF::create(row.key.c_str(), "arial", scale * kKeyFontSize);
    keyLabel->setAnchorPoint(ccp(0.0f, 0.5f));
    keyLabel->setPosition(ccp(scale * kTextLeft, scale * kKeyTop));
    keyLabel->setHorizontalAlignment(kCCTextAlignmentLeft);
    keyLabel->setContentSize(CCSize(scale * kTextWidth, scale * kKeyHeight));
    keyLabel->setDimensions(keyLabel->getContentSize());
    keyLabel->setTag(kTagKeyLabel);
    cell->addChild(keyLabel);

    CCLabelTTF* valueLabel = CCLabelTTF::create(row.value.c_str(), "arial", scale * kValueFontSize);
    valueLabel->setAnchorPoint(ccp(0.0f, 1.0f));
    valueLabel->setPosition(ccp(scale * kTextLeft, scale * kValueTop));
    valueLabel->setHorizontalAlignment(kCCTextAlignmentLeft);
    valueLabel->setContentSize(CCSize(scale * kTextWidth, scale * kValueHeight));
    valueLabel->setDimensions(valueLabel->getContentSize());
    valueLabel->setTag(kTagValueLabel);
    cell->addChild(valueLabel);

    // Thin rule under each row: full width, two design pixels high.
    CCLayerColor* separator = CCLayerColor::create(kSeparatorColor);
    separator->setContentSize(CCSize(scale * kDesignWidth, scale + scale));
    separator->ignoreAnchorPointForPosition(false);
    separator->setAnchorPoint(ccp(0.0f, 0.5f));
    separator->setPosition(ccp(0.0f, scale * kSeparatorY));
    cell->addChild(separator);

    return cell;
}

unsigned int EnvTableLayer::numberOfCellsInTableView(CCTableView*) {
    return static_cast<unsigned int>(m_rows.size());
}

void EnvTableLayer::scrollViewDidScroll(CCScrollView*) {}

void EnvTableLayer::scrollViewDidZoom(CCScrollView*) {}

// orig 001c766c
void EnvTableLayer::tableCellTouched(CCTableView*, CCTableViewCell* cell) {
    CCLabelTTF* keyLabel = static_cast<CCLabelTTF*>(cell->getChildByTag(kTagKeyLabel));
    CCLabelTTF* valueLabel = static_cast<CCLabelTTF*>(cell->getChildByTag(kTagValueLabel));
    std::string key = keyLabel->getString();
    std::string value = valueLabel->getString();
    // Deviation: the original called the std::function unchecked (abort if a caller set an empty one).
    if (m_callback) m_callback(key, value);
}
