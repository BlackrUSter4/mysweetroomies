#include "BackgroundRender.h"

#include "BaseFilter.h"
#include "MiToast.h"
#include "ResourceManager.h"

USING_NS_CC;

namespace {
const int kBackgroundTag = 1;
const int kParticleTag = 2;
}  // namespace

// orig 001de3d0
BackgroundRender *BackgroundRender::create()
{
    BackgroundRender *layer = new BackgroundRender();
    if (!layer->init()) {
        delete layer;
        return nullptr;
    }
    layer->autorelease();
    return layer;
}

// orig 001e3ce0
BackgroundRender::~BackgroundRender() {}

// orig 001e3fc0
bool BackgroundRender::init()
{
    // The original also called removeFromParentAndCleanup(true) here, a no-op on a node without a parent.
    return CCLayer::init();
}

// orig 001e3d58
void BackgroundRender::drawImage(std::shared_ptr<ViewBackgroundData> background)
{
    if (background->type == ViewBackgroundType_None) {
        // Only a filter without a picture gets here; the original warned in debug builds and drew nothing.
        if (background->filter.hasFilter()) MiToast::debug("Sorry! フィルタ不具合中.", MiToast::DURATION_SHORT);
        return;
    }

    removeChildByTag(kBackgroundTag);

    CCNode *node = nullptr;
    if (background->type != ViewBackgroundType_Reset && ResourceManager::getInstance()->isExistFile(background->path)) {
        CCSprite *sprite = background->filter.hasFilter() ? background->filter.createFilter(background->path)
                                                          : CCSprite::create(background->path.c_str());
        if (sprite != nullptr) {  // the original assumed the image always loads
            // Fit the picture to the canvas width; it stays centred on the layer origin (sprite anchor 0.5).
            sprite->setScale(getContentSize().width / sprite->getContentSize().width);
            node = sprite;
        }
    }
    if (node == nullptr) {
        // Reset backgrounds, and pictures missing from the resource cache, become a window-sized colour layer
        // centred on the origin like a sprite would be.
        CCLayerColor *color = CCLayerColor::create(background->color);
        color->ignoreAnchorPointForPosition(false);
        color->setAnchorPoint(CCPoint(0.5f, 0.5f));
        color->setPosition(CCPoint(0.0f, 0.0f));
        node = color;
    }
    // The original called removeFromParentAndCleanup(true) on the new node first (a no-op: it has no parent yet).
    addChild(node, 0, kBackgroundTag);
}

// orig 001e3ebc
void BackgroundRender::onMainRender(std::shared_ptr<ViewData> current) {}

// orig 001e3ec0
void BackgroundRender::onPreRender(std::shared_ptr<ViewData> previous, std::shared_ptr<ViewData> current)
{
    if (!current->background) return;
    drawImage(current->background);

    const std::shared_ptr<std::string> particle = current->background->particle;
    if (!particle) return;

    removeChildByTag(kParticleTag);
    // The old emitter went away with its tag. When the new file was missing the original still called stopSystem()
    // on it, i.e. on a released object; forgetting it is all that is left to do.
    m_particle = nullptr;
    if (!CCFileUtils::sharedFileUtils()->isFileExist(*particle)) return;
    m_particle = CCParticleSystemQuad::create(particle->c_str());
    m_particle->setPosition(CCPoint(0.0f, getContentSize().height * 3.0f * 0.25f));
    addChild(m_particle, kParticleTag, kParticleTag);
}
