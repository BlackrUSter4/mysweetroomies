#include "PersonRender.h"

#include "BaseFilter.h"
#include "FilterData.h"

USING_NS_CC;

namespace {
const char *const kMouthLayerName = "05_mouth";
const float kReferenceCanvasWidth = 480.0f;

// Sprite of `path`, through the person's filter when there is one.
CCSprite *createLayerSprite(const std::string &path, ViewPersonData *person)
{
    if (person != nullptr && person->filter.hasFilter()) return person->filter.createFilter(path);
    return CCSprite::create(path.c_str());
}
}  // namespace

// orig 001de44c
PersonRender *PersonRender::create()
{
    PersonRender *layer = new PersonRender();
    if (!layer->init()) {
        delete layer;
        return nullptr;
    }
    layer->autorelease();
    return layer;
}

// orig 001e4dd4
PersonRender::~PersonRender() {}

// orig 001e55d8
bool PersonRender::init()
{
    if (!CCLayer::init()) return false;
    resetCurrentMaximumContentSize();
    m_unusedSprites.clear();
    return true;
}

// orig 001e4e54
float PersonRender::getCanvasScale()
{
    return getContentSize().width / kReferenceCanvasWidth;
}

// orig 001e4e74
void PersonRender::setCurrentMaximumContentSize(CCSize size)
{
    if (m_currentMaximumContentSize.width < size.width || m_currentMaximumContentSize.height < size.height)
        m_currentMaximumContentSize = size;
}

// orig 001e4ea4
CCSize PersonRender::getCurrentMaximumContentSize()
{
    return m_currentMaximumContentSize;
}

// orig 001e4eb4
void PersonRender::resetCurrentMaximumContentSize()
{
    m_currentMaximumContentSize = CCDirector::sharedDirector()->getWinSize();
}

// orig 001e50a0
void PersonRender::resetData()
{
    removeAllChildren();
}

// orig 001e4ee2
void PersonRender::addOffsetLayerData(std::shared_ptr<OffsetLayerData> layer, std::shared_ptr<ViewPersonData> person)
{
    // The same file may be cached from a scene with a different filter.
    CCTextureCache::sharedTextureCache()->removeTextureForKey(layer->path.c_str());
    CCSprite *sprite = createLayerSprite(layer->path, person.get());
    if (sprite == nullptr) return;  // the original assumed every image loads

    const float scale = getCanvasScale();
    const CCSize spriteSize = sprite->getContentSize();
    setCurrentMaximumContentSize(CCSize(scale * spriteSize.width, scale * spriteSize.height));

    // getOffsetPoint() is relative to the layer size, measured from the layer origin (= canvas centre).
    const CCSize layerSize = getContentSize();
    const CCPoint offset = layer->getOffsetPoint(spriteSize);
    sprite->setPosition(CCPoint(offset.x * layerSize.width, offset.y * layerSize.height));
    sprite->setScale(scale);
    addChild(sprite);
}

// orig 001e5230
void PersonRender::addFrameAnimationData(std::shared_ptr<FrameAnimationData> layer,
                                         std::shared_ptr<ViewPersonData> person, int tag)
{
    if (layer->frames.empty()) return;  // the original threw out_of_range from frames.at(0)

    // A mouth rests on its third image (closed); everything else starts on its first. Either way the sprite is
    // placed with the first frame's offset.
    std::shared_ptr<OffsetLayerData> restingFrame = layer->frames[0];
    if (layer->frames.size() >= 3 && layer->name == kMouthLayerName) restingFrame = layer->frames[2];

    CCSprite *sprite = createLayerSprite(restingFrame->path, person.get());
    if (sprite == nullptr) return;

    const CCSize layerSize = getContentSize();
    const CCPoint offset = layer->frames[0]->getOffsetPoint(sprite->getContentSize());
    sprite->setPosition(CCPoint(offset.x * layerSize.width, offset.y * layerSize.height));
    sprite->setScale(getCanvasScale());
    addChild(sprite, tag, tag);
}

// orig 001e5008
void PersonRender::runFrameAnimation(std::shared_ptr<FrameAnimationData> layer, std::shared_ptr<ViewPersonData> person,
                                     int tag)
{
    CCSprite *sprite = dynamic_cast<CCSprite *>(getChildByTag(tag));
    if (sprite == nullptr) return;  // the original ran the action on a null node

    // The mouth talks once through its sequence per line; eyes keep blinking.
    const int loops = layer->name == kMouthLayerName ? 1 : -1;
    FilterData filter;
    if (person) filter = person->filter;
    sprite->runAction(CCAnimate::create(layer->toCCAnimation(filter, loops)));
}

// orig 001e53a0
void PersonRender::onPreRender(std::shared_ptr<ViewPersonData> previous, std::shared_ptr<ViewPersonData> current)
{
    if (!current || current->layers.empty()) return;

    removeAllChildren();
    resetCurrentMaximumContentSize();
    m_unusedSprites.clear();

    for (size_t i = 0; i < current->layers.size(); ++i) {
        const std::shared_ptr<LayerData> &layer = current->layers[i];
        if (!layer) continue;
        if (instanceof<FrameAnimationData>(layer)) {
            addFrameAnimationData(std::dynamic_pointer_cast<FrameAnimationData>(layer), current, static_cast<int>(i));
        } else if (std::shared_ptr<OffsetLayerData> still = std::dynamic_pointer_cast<OffsetLayerData>(layer)) {
            addOffsetLayerData(still, current);
        }
    }

    const CCSize size = getContentSize();
    switch (current->position) {
    case ViewPersonPosition_Left:
        setPosition(CCPoint(size.width * -0.25f, 0.0f));
        break;
    case ViewPersonPosition_Right:
        setPosition(CCPoint(size.width * 0.25f, 0.0f));
        break;
    default:
        setPosition(CCPoint(0.0f, 0.0f));
        break;
    }
}

// orig 001e513c
void PersonRender::onMainRender(std::shared_ptr<ViewPersonData> current)
{
    if (!current) return;
    for (size_t i = 0; i < current->layers.size(); ++i) {
        const std::shared_ptr<LayerData> &layer = current->layers[i];
        if (layer && instanceof<FrameAnimationData>(layer))
            runFrameAnimation(std::dynamic_pointer_cast<FrameAnimationData>(layer), current, static_cast<int>(i));
    }
}
