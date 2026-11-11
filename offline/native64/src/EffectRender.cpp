#include "EffectRender.h"

#include "SceneEventListener.h"
#include "SceneManager.h"

USING_NS_CC;

namespace {
const char *const kListenerName = "effect_render";  // also the scene-lock owner
const char *const kConversationPlist = "Framework/ui/conversation.plist";
const char *const kConversationTexture = "Framework/ui/conversation.png";
const char *const kEndMarkEndFrame = "fin.png";
const char *const kEndMarkContinueFrame = "continued.png";

const int kManpuTag = 0;
const int kParticleTag = 1;
const int kEndMarkTag = 2;

// Manpu and particle positions are authored for a 480 px wide screen, end marks for 640 px.
const float kEffectReferenceWidth = 480.0f;
const float kEndMarkReferenceWidth = 640.0f;

void reportDone(SceneEventType event)
{
    SceneManager::getInstance()->doneListenerEvent(event, kListenerName);
}

std::shared_ptr<ViewData> viewOf(const std::shared_ptr<SceneData> &scene)
{
    return scene ? scene->view : std::shared_ptr<ViewData>();
}
}  // namespace

// orig 001de4e8
EffectRender *EffectRender::create()
{
    EffectRender *layer = new EffectRender();
    if (!layer->init()) {
        delete layer;
        return nullptr;
    }
    layer->autorelease();
    return layer;
}

// orig 001e3fdc
EffectRender::~EffectRender()
{
    // Deviation: the original left its listener (which captures `this`) registered. A story always ends through
    // SceneManager's Finish phase, which drops every listener, before the next ConversationScene exists; removing it
    // here only matters for a story that was abandoned, where the stale listener would call into freed memory.
    SceneManager::getInstance()->removeListener(kListenerName);
}

// orig 001e4ac0
bool EffectRender::init()
{
    if (!CCLayer::init()) return false;

    CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile(kConversationPlist, kConversationTexture);

    SceneManager::getInstance()->addListener(
        kListenerName,
        std::make_shared<SceneEventListener>(
            [](const std::vector<std::shared_ptr<SceneData>> &) { reportDone(SceneEventType_Setup); },
            [this](std::shared_ptr<SceneData> current) {
                onMainRender(viewOf(current));
                reportDone(SceneEventType_MainRender);
            },
            [this](std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current) {
                onPreRender(viewOf(previous), viewOf(current));
                reportDone(SceneEventType_PreRender);
            },
            [this](std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current) {
                onPreEffect(viewOf(previous), viewOf(current));
                reportDone(SceneEventType_PreEffect);
            },
            [](std::shared_ptr<SceneData>) { reportDone(SceneEventType_PostEffect); },
            []() { reportDone(SceneEventType_Finish); }));
    return true;
}

// orig 001e47a4
void EffectRender::onPreRender(std::shared_ptr<ViewData> previous, std::shared_ptr<ViewData> current)
{
    // Particles stay until a scene replaces or stops them (onRenderParticle).
    removeChildByTag(kManpuTag);
    removeChildByTag(kEndMarkTag);
}

// orig 001e47c0
void EffectRender::onPreEffect(std::shared_ptr<ViewData> previous, std::shared_ptr<ViewData> current) {}

// orig 001e49cc
void EffectRender::onMainRender(std::shared_ptr<ViewData> current)
{
    if (!current || !current->effect) return;
    onRenderParticle(current->effect);
    onRenderManpu(current->effect);
    // Held until the end mark finished playing; onRenderEndMark releases it at once when there is none.
    SceneManager::getInstance()->lockScene(kListenerName);
    onRenderEndMark(current->effect->endMark);
}

// orig 001e43f8
void EffectRender::onRenderParticle(std::shared_ptr<ViewEffectData> effect)
{
    if (effect->stopParticles) {
        removeAllChildren();
        return;
    }
    if (effect->particles.empty()) return;

    removeAllChildren();
    const float scale = CCDirector::sharedDirector()->getWinSize().width / kEffectReferenceWidth;
    for (const ExVisualParticleSpec &spec : effect->particles) {
        CCParticleSystemQuad *particle = CCParticleSystemQuad::create(spec.path.c_str());
        if (particle == nullptr) continue;  // the original assumed the .plist always loads
        particle->setAnchorPoint(CCPoint(0.5f, 0.5f));
        // Spec coordinates are measured from the layer's bottom right corner, x growing leftwards and y upwards
        // (the original's arithmetic, in double precision).
        const CCSize size = getContentSize();
        const double x = static_cast<double>(size.width * 0.5f) - static_cast<double>(scale) * spec.x;
        const double y = static_cast<double>(scale) * spec.y - static_cast<double>(size.height * 0.5f);
        particle->setPosition(CCPoint(static_cast<float>(x), static_cast<float>(y)));
        particle->setScale(scale);
        addChild(particle, kParticleTag, kParticleTag);
    }
}

// orig 001e41c4
void EffectRender::onRenderManpu(std::shared_ptr<ViewEffectData> effect)
{
    const std::shared_ptr<ManpuData> manpu = effect->manpu;
    if (!manpu || manpu->paths.empty()) return;

    CCSprite *sprite = CCSprite::create(manpu->paths[0].c_str());
    if (sprite == nullptr) return;  // the original assumed the image always loads

    const CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    const CCSize layerSize = getContentSize();
    const float scale = winSize.width / kEffectReferenceWidth;
    sprite->setScale(scale);
    const CCSize spriteSize = sprite->getContentSize() * scale;

    // Top corner of the chosen side, the sprite fully inside the layer (origin at the layer centre).
    CCPoint target;
    if (manpu->location == ExVisualManpuLocation_Left)
        target.x = -layerSize.width * 0.5f + spriteSize.width * 0.5f;
    else if (manpu->location == ExVisualManpuLocation_Right)
        target.x = layerSize.width * 0.5f - spriteSize.width * 0.5f;
    else
        return;
    target.y = layerSize.height * 0.5f - spriteSize.height * 0.5f;
    sprite->setPosition(target);

    CCAnimation *animation = CCAnimation::create();
    for (const std::string &path : manpu->paths) animation->addSpriteFrameWithFileName(path.c_str());
    animation->setDelayPerUnit(static_cast<float>(manpu->transitionTime / static_cast<double>(manpu->paths.size())));
    animation->setRestoreOriginalFrame(true);
    animation->setLoops(-1);

    // Pops in: fades in while rising 20 px into place, then cycles its images forever.
    CCAction *appear = CCSequence::create(CCFadeOut::create(0.0f), CCShow::create(),
                                          CCSpawn::create(CCFadeIn::create(0.3f), CCMoveTo::create(0.3f, target), NULL),
                                          CCAnimate::create(animation), NULL);
    sprite->setPosition(CCPoint(target.x, target.y - 20.0f));
    sprite->runAction(CCHide::create());
    sprite->runAction(appear);
    addChild(sprite, kManpuTag, kManpuTag);
}

// orig 001e49ac
void EffectRender::onRenderEndMark(ExVisualEndMarkType type)
{
    switch (type) {
    case ExVisualEndMarkType_Continue:
        renderEndMarkContinue();
        break;
    case ExVisualEndMarkType_End:
        renderEndMarkEnd();
        break;
    case ExVisualEndMarkType_None:
        onFinishEndMark();
        break;
    }
}

// orig 001e4674
void EffectRender::renderEndMarkContinue()
{
    CCSprite *mark = CCSprite::createWithSpriteFrameName(kEndMarkContinueFrame);
    const CCSize layerSize = getContentSize();
    const CCPoint target(0.0f, 0.0f);

    mark->setScale(CCDirector::sharedDirector()->getWinSize().width / kEndMarkReferenceWidth);
    mark->setAnchorPoint(CCPoint(0.5f, 1.0f));
    mark->setPosition(target);
    mark->setVisible(false);

    // Invisibly parks one layer width to the right, then slides in from there while fading in.
    mark->runAction(CCSequence::create(
        CCFadeOut::create(0.0f), CCMoveTo::create(1.0f, CCPoint(target.x + layerSize.width, target.y)),
        CCShow::create(), CCSpawn::create(CCMoveTo::create(0.5f, target), CCFadeIn::create(0.5f), NULL),
        CCDelayTime::create(1.0f), CCCallFunc::create(this, callfunc_selector(EffectRender::onFinishEndMark)),
        NULL));
    addChild(mark, 1, kEndMarkTag);
}

// orig 001e452c
void EffectRender::renderEndMarkEnd()
{
    CCSprite *mark = CCSprite::createWithSpriteFrameName(kEndMarkEndFrame);
    const CCSize layerSize = getContentSize();
    const CCPoint target(0.0f, layerSize.height * 0.5f);

    const float scale = CCDirector::sharedDirector()->getWinSize().width / kEndMarkReferenceWidth;
    mark->setScale(scale);
    mark->setAnchorPoint(CCPoint(0.5f, 1.0f));
    mark->setPosition(target);
    mark->setVisible(false);

    // Invisibly parks 40 design pixels above the top edge, then drops into place while fading in.
    mark->runAction(CCSequence::create(
        CCFadeOut::create(0.0f), CCMoveTo::create(1.0f, CCPoint(target.x, scale * 40.0f + target.y)),
        CCShow::create(), CCSpawn::create(CCMoveTo::create(0.5f, target), CCFadeIn::create(0.5f), NULL),
        CCDelayTime::create(1.0f), CCCallFunc::create(this, callfunc_selector(EffectRender::onFinishEndMark)),
        NULL));
    addChild(mark, 1, kEndMarkTag);
}

// orig 001e4994
void EffectRender::onFinishEndMark()
{
    SceneManager::getInstance()->unlockScene(kListenerName);
}
