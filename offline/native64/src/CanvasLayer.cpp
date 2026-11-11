#include "CanvasLayer.h"

#include "BackgroundRender.h"
#include "EffectRender.h"
#include "HalfGradientLayer.h"
#include "MiToast.h"
#include "PersonRender.h"
#include "SceneManager.h"

USING_NS_CC;

namespace {
const char *const kListenerName = "canvas_layer";
const int kCameraActionTag = 0;  // the Spawn of camera pan + shake, run on the canvas itself
const size_t kPersonSlots = 3;

// z order == tag for every child (see the header).
const int kBackgroundTag = 0;
const int kPersonTag = 4;
const int kEffectTag = 8;

// The three curtain sets of a fade target: a plain curtain for "even" fades and a HalfGradientLayer pair for the
// centre fades. `upper` is the pair member anchored at its top edge with scale +1, `lower` the one with scale -1.
struct CurtainTags {
    int plain;
    int lower;
    int upper;
};
const CurtainTags kBackgroundCurtains = {1, 2, 3};
const CurtainTags kPersonCurtains = {5, 6, 7};
const CurtainTags kAllCurtains = {9, 10, 11};

// Fade target -> curtains; false for a target the original had no curtain for.
bool curtainsFor(ExVisualFadeOn on, CurtainTags &tags)
{
    switch (on) {
    case ExVisualFadeOn_Bg:
        tags = kBackgroundCurtains;
        return true;
    case ExVisualFadeOn_Fg:
        tags = kPersonCurtains;
        return true;
    case ExVisualFadeOn_Both:
        tags = kAllCurtains;
        return true;
    default:
        return false;
    }
}

void reportDone(SceneEventType event)
{
    SceneManager::getInstance()->doneListenerEvent(event, kListenerName);
}

// orig updateMaxSize 001de786: grows `maximum` to `candidate` when either dimension is larger.
void updateMaxSize(CCSize &maximum, const CCSize &candidate)
{
    if (maximum.width < candidate.width || maximum.height < candidate.height) maximum = candidate;
}

// orig resetCurtainAction 001ded54
void resetCurtainAction(CCNode *canvas, int tag)
{
    CCNode *curtain = canvas->getChildByTag(tag);
    curtain->stopAllActions();
    curtain->runAction(CCHide::create());
}

ccColor3B rgbOf(const ccColor4B &color)
{
    return ccc3(color.r, color.g, color.b);
}
}  // namespace

// orig 001c8a78
CanvasLayer *CanvasLayer::create()
{
    CanvasLayer *layer = new CanvasLayer();
    if (!layer->init()) {
        delete layer;
        return nullptr;
    }
    layer->autorelease();
    return layer;
}

// orig 001dedf8
CanvasLayer::~CanvasLayer()
{
    SceneManager::getInstance()->removeListener(kListenerName);
}

// orig 001dfe54
bool CanvasLayer::init()
{
    if (!CCLayer::init()) return false;
    // The original also called removeFromParentAndCleanup(true) here, a no-op on a node without a parent.
    initUi();
    SceneManager::getInstance()->addListener(
        kListenerName, std::make_shared<SceneEventListener>(onSetup(), onMainRender(), onPreRender(), onPreEffect(),
                                                            onPostEffect(), onFinishScene()));
    return true;
}

// orig 001dfcf8
void CanvasLayer::initUi()
{
    const CCSize size = getCanvasSize();

    m_backgroundRender = BackgroundRender::create();
    m_backgroundRender->setContentSize(size);
    addChild(m_backgroundRender, kBackgroundTag, kBackgroundTag);
    addCurtain(kBackgroundCurtains.plain);
    addDoubleCurtain(kBackgroundCurtains.lower, kBackgroundCurtains.upper);

    for (size_t i = 0; i < kPersonSlots; ++i) {
        PersonRender *person = PersonRender::create();
        m_personRenders.push_back(person);
        person->setContentSize(size);
        addChild(person, kPersonTag, kPersonTag);
    }
    addCurtain(kPersonCurtains.plain);
    addDoubleCurtain(kPersonCurtains.lower, kPersonCurtains.upper);

    m_effectRender = EffectRender::create();
    m_effectRender->setContentSize(size);
    addChild(m_effectRender, kEffectTag, kEffectTag);
    addCurtain(kAllCurtains.plain);
    addDoubleCurtain(kAllCurtains.lower, kAllCurtains.upper);
}

// orig 001de7b0
SceneEventListener::SetupHandler CanvasLayer::onSetup()
{
    return [this](const std::vector<std::shared_ptr<SceneData>> &scenes) { handleOnSetup(scenes); };
}

// orig 001de7e8
SceneEventListener::SceneHandler CanvasLayer::onMainRender()
{
    return [this](std::shared_ptr<SceneData> current) { handleOnMainRender(current); };
}

// orig 001de820
SceneEventListener::SceneHandler CanvasLayer::onPostEffect()
{
    return [this](std::shared_ptr<SceneData> previous) { handleOnPostEffect(previous); };
}

// orig 001de858
SceneEventListener::TransitionHandler CanvasLayer::onPreRender()
{
    return [this](std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current) {
        handleOnPreRender(previous, current);
    };
}

// orig 001de890
SceneEventListener::TransitionHandler CanvasLayer::onPreEffect()
{
    return [this](std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current) {
        handleOnPreEffect(previous, current);
    };
}

// orig 001de8c8
SceneEventListener::FinishHandler CanvasLayer::onFinishScene()
{
    return [this]() { handleOnFinishScene(); };
}

// orig 001dee8c
void CanvasLayer::handleOnSetup(const std::vector<std::shared_ptr<SceneData>> &scenes)
{
    reportDone(SceneEventType_Setup);
}

// orig 001df07c
void CanvasLayer::handleOnFinishScene()
{
    reportDone(SceneEventType_Finish);
}

// orig 001df570
void CanvasLayer::onPreRenderFinished(CCObject *sender)
{
    reportDone(SceneEventType_PreRender);
}

// orig 001df0a0
void CanvasLayer::onPreEffectFinished(CCObject *sender)
{
    reportDone(SceneEventType_PreEffect);
}

// orig 001df818
void CanvasLayer::onPostEffectFinished(CCObject *sender)
{
    reportDone(SceneEventType_PostEffect);
}

// orig 001df588
void CanvasLayer::handleOnPreRender(std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current)
{
    const std::shared_ptr<ViewData> view = current->view;
    const std::shared_ptr<ViewData> previousView = previous ? previous->view : std::shared_ptr<ViewData>();

    if (!view->persons.empty()) resetCanvasSize();
    resetAction();
    // Without new characters the camera stays where the last pan left it.
    if (view->persons.empty())
        setPosition(m_currentPosition);
    else
        resetPosition();

    if (view->background) m_backgroundRender->onPreRender(previousView, view);

    if (view->persons.empty()) {
        if (view->resetPersons)
            for (PersonRender *person : m_personRenders) person->resetData();
    } else {
        for (size_t i = 0; i < kPersonSlots; ++i) {
            if (i < view->persons.size() && !view->resetPersons) {
                std::shared_ptr<ViewPersonData> before;
                if (previousView && !previousView->resetPersons && i < previousView->persons.size())
                    before = previousView->persons[i];
                m_personRenders.at(i)->onPreRender(before, view->persons[i]);
            } else {
                m_personRenders.at(i)->resetData();
            }
        }
    }
    onPreRenderFinished(this);
}

// orig 001df0b8
void CanvasLayer::handleOnPreEffect(std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current)
{
    if (!previous) {  // the first scene appears without a fade-in
        onPreEffectFinished(this);
        return;
    }
    // The fade-in uses the fade of the scene being left, mirroring what its PostEffect faded out to.
    const std::shared_ptr<FadeData> fade = previous->view->effect->fade;
    CurtainTags tags;
    if (!fade || !curtainsFor(fade->on, tags)) {
        onPreEffectFinished(this);
        return;
    }

    const float duration = fade->duration;
    switch (fade->type) {
    case ExVisualFadeType_Even: {
        CCLayerColor *curtain = static_cast<CCLayerColor *>(getChildByTag(tags.plain));
        curtain->stopAllActions();
        CCAction *fadeIn = CCSequence::create(
            CCShow::create(), CCFadeOut::create(duration), CCHide::create(),
            CCCallFuncO::create(this, callfuncO_selector(CanvasLayer::onPreEffectFinished), nullptr), NULL);
        curtain->setColor(rgbOf(fade->color));
        curtain->setOpacity(fade->color.a);
        curtain->runAction(fadeIn);
        break;
    }
    case ExVisualFadeType_FromCenter:
    case ExVisualFadeType_ToCenter: {
        HalfGradientLayer *upper = static_cast<HalfGradientLayer *>(getChildByTag(tags.upper));
        HalfGradientLayer *lower = static_cast<HalfGradientLayer *>(getChildByTag(tags.lower));
        upper->stopAllActions();  // only this one, as in the original
        // Both halves start fully closed and fold away (scale y towards 0).
        CCAction *openUpper = CCSequence::create(CCScaleTo::create(0.0f, 1.0f, 1.0f), CCShow::create(),
                                                 CCScaleTo::create(duration, 1.0f, 0.0f), CCHide::create(), NULL);
        CCAction *openLower = CCSequence::create(
            CCScaleTo::create(0.0f, 1.0f, -1.0f), CCShow::create(), CCScaleTo::create(duration, 1.0f, 0.0f),
            CCHide::create(), CCCallFuncO::create(this, callfuncO_selector(CanvasLayer::onPreEffectFinished), nullptr),
            NULL);
        upper->setLayerColor(fade->color);
        lower->setLayerColor(fade->color);
        if (fade->type == ExVisualFadeType_FromCenter) {
            // Hinged at the top and bottom edges: the gap opens in the middle and spreads outwards.
            const CCSize size = getCanvasSize();
            upper->setPosition(CCPoint(0.0f, size.height * 0.5f));
            lower->setPosition(CCPoint(0.0f, -size.height * 0.5f));
        } else {
            // Hinged at the centre line: the picture appears at the edges first.
            upper->setPosition(CCPoint(0.0f, 0.0f));
            lower->setPosition(CCPoint(0.0f, 0.0f));
        }
        upper->runAction(openUpper);
        lower->runAction(openLower);
        break;
    }
    default:
        // Deviation: the original never reported PreEffect done for a fade of type "none", stalling the story.
        onPreEffectFinished(this);
        break;
    }
}

// orig 001deeb0
void CanvasLayer::handleOnMainRender(std::shared_ptr<SceneData> current)
{
    const std::shared_ptr<ViewData> view = current->view;
    if (!view->persons.empty()) resetCanvasSize();

    // The camera pans over the largest of the canvas and the characters' sprites.
    CCSize contentSize = getCanvasSize();
    if (view->background) m_backgroundRender->onMainRender(view);

    if (view->persons.empty()) {
        for (PersonRender *person : m_personRenders) {
            if (view->resetPersons)
                person->resetData();
            else
                updateMaxSize(contentSize, person->getCurrentMaximumContentSize());
        }
    } else {
        for (size_t i = 0; i < kPersonSlots; ++i) {
            PersonRender *person = m_personRenders.at(i);
            if (i < view->persons.size() && !view->resetPersons) {
                person->onMainRender(view->persons[i]);
                updateMaxSize(contentSize, person->getCurrentMaximumContentSize());
            } else {
                person->resetData();
            }
        }
    }

    onRenderAction(view, contentSize);
    reportDone(SceneEventType_MainRender);
}

// orig 001df830
void CanvasLayer::handleOnPostEffect(std::shared_ptr<SceneData> previous)
{
    const std::shared_ptr<FadeData> fade = previous->view->effect->fade;
    CurtainTags tags;
    if (!fade || !curtainsFor(fade->on, tags)) {
        onPostEffectFinished(this);
        return;
    }

    const float duration = fade->duration;
    switch (fade->type) {
    case ExVisualFadeType_Even: {
        // Fades the curtain in and leaves it up; the next PreEffect takes it down again.
        CCLayerColor *curtain = static_cast<CCLayerColor *>(getChildByTag(tags.plain));
        curtain->stopAllActions();
        CCAction *fadeOut = CCSequence::create(
            CCFadeOut::create(0.0f), CCShow::create(), CCFadeIn::create(duration),
            CCCallFuncO::create(this, callfuncO_selector(CanvasLayer::onPostEffectFinished), nullptr), NULL);
        curtain->setColor(rgbOf(fade->color));
        curtain->setOpacity(fade->color.a);
        curtain->runAction(fadeOut);
        break;
    }
    case ExVisualFadeType_FromCenter:
    case ExVisualFadeType_ToCenter: {
        HalfGradientLayer *upper = static_cast<HalfGradientLayer *>(getChildByTag(tags.upper));
        HalfGradientLayer *lower = static_cast<HalfGradientLayer *>(getChildByTag(tags.lower));
        upper->stopAllActions();  // only this one, as in the original
        // The reverse of PreEffect: both halves unfold from nothing to fully closed and stay visible.
        CCAction *closeUpper = CCSequence::create(CCScaleTo::create(0.0f, 1.0f, 0.0f), CCShow::create(),
                                                  CCScaleTo::create(duration, 1.0f, 1.0f), NULL);
        CCAction *closeLower = CCSequence::create(
            CCScaleTo::create(0.0f, 1.0f, 0.0f), CCShow::create(), CCScaleTo::create(duration, 1.0f, -1.0f),
            CCCallFuncO::create(this, callfuncO_selector(CanvasLayer::onPostEffectFinished), nullptr), NULL);
        upper->setLayerColor(fade->color);
        lower->setLayerColor(fade->color);
        if (fade->type == ExVisualFadeType_FromCenter) {
            const CCSize size = getCanvasSize();
            upper->setPosition(CCPoint(0.0f, size.height * 0.5f));
            lower->setPosition(CCPoint(0.0f, -size.height * 0.5f));
        } else {
            upper->setPosition(CCPoint(0.0f, 0.0f));
            lower->setPosition(CCPoint(0.0f, 0.0f));
        }
        upper->runAction(closeUpper);
        lower->runAction(closeLower);
        break;
    }
    default:
        // Deviation: the original never reported PostEffect done for a fade of type "none", stalling the story.
        onPostEffectFinished(this);
        break;
    }
}

// orig 001de968
void CanvasLayer::onRenderAction(std::shared_ptr<ViewData> view, const CCSize &contentSize)
{
    const CCSize canvas = getCanvasSize();
    CCArray *actions = CCArray::create();
    const std::shared_ptr<ViewEffectData> effect = view->effect;

    if (effect && effect->shake != ExVisualShakeType_None) {
        // Amplitude as a fraction of the canvas width; one cycle = right, two steps left, back.
        float amplitude = canvas.width / 10.0f;
        float step = 0.1f;
        unsigned int cycles = 1;
        switch (effect->shake) {
        case ExVisualShakeType_Soft:
            amplitude = canvas.width / 30.0f;
            step = 0.08f;
            cycles = 1;
            break;
        case ExVisualShakeType_Medium:
            amplitude = canvas.width / 20.0f;
            step = 0.08f;
            cycles = 2;
            break;
        case ExVisualShakeType_Hard:
            amplitude = canvas.width / 5.0f;
            step = 0.08f;
            cycles = 3;
            break;
        default:  // unknown strengths: the original's fallback
            break;
        }
        CCArray *cycle = CCArray::create();
        cycle->addObject(CCMoveBy::create(step, CCPoint(amplitude, 0.0f)));
        cycle->addObject(CCMoveBy::create(step + step, CCPoint(-amplitude + -amplitude, 0.0f)));
        cycle->addObject(CCMoveBy::create(step, CCPoint(amplitude, 0.0f)));
        CCArray *shake = CCArray::create();
        shake->addObject(CCRepeat::create(CCSequence::create(cycle), cycles));
        shake->addObject(CCCallFuncO::create(this, callfuncO_selector(CanvasLayer::onShakeFinished), nullptr));
        actions->addObject(CCSequence::create(shake));
    }

    if (effect && effect->camera != ExVisualCameraType_None) {
        // (The original also built the camera type's JSON name here and never used it.)
        const float screenScale = CCDirector::sharedDirector()->getWinSize().width / 480.0f;
        const ExVisualCameraType camera = effect->camera;
        const bool vertical = camera == ExVisualCameraType_UpDown || camera == ExVisualCameraType_DownUp;
        // up_down and right_left start at the resting position and pan by a negative offset.
        const bool towardsNegative = camera == ExVisualCameraType_UpDown || camera == ExVisualCameraType_RightLeft;
        const float extent = vertical ? contentSize.height : contentSize.width;

        // Half of what the content sticks out beyond the screen along the pan axis, signed.
        const float offset = (extent - screenScale * 480.0f) * 0.5f * (towardsNegative ? -1.0f : 1.0f);
        if (offset == 0.0f) MiToast::warn("camera direction and size not matched!", MiToast::DURATION_SHORT);
        // Three seconds per extra 480 px of content (unscaled, as in the original).
        const float duration = (extent - 480.0f) / 480.0f * 3.0f;

        const CCPoint home = getDefaultPosition();
        CCPoint from;
        CCPoint to;
        if (vertical) {
            from = towardsNegative ? CCPoint(home.x, home.y) : CCPoint(home.x, offset + offset + home.y);
            to = towardsNegative ? CCPoint(home.x, home.y - (offset + offset)) : CCPoint(home.x, home.y);
        } else {
            from = towardsNegative ? CCPoint(offset + offset + home.x, home.y) : CCPoint(home.x, home.y);
            to = towardsNegative ? CCPoint(home.x, home.y) : CCPoint(home.x - (offset + offset), home.y);
        }

        CCFiniteTimeAction *jumpToStart = CCMoveTo::create(0.0f, from);
        CCFiniteTimeAction *pan = CCMoveTo::create(duration, to);
        m_currentPosition = to;
        actions->addObject(CCSequence::create(
            jumpToStart, CCDelayTime::create(0.1f), pan,
            CCCallFuncO::create(this, callfuncO_selector(CanvasLayer::onCameraAnimationFinished), nullptr), NULL));
    }

    if (actions->count() != 0) {
        CCAction *all = CCSpawn::create(actions);
        all->setTag(kCameraActionTag);
        runAction(all);
    }
}

// orig 001ded72
void CanvasLayer::resetAction()
{
    stopActionByTag(kCameraActionTag);
    const int curtains[] = {kBackgroundCurtains.plain, kPersonCurtains.plain,  kAllCurtains.plain,
                            kBackgroundCurtains.lower, kBackgroundCurtains.upper, kPersonCurtains.lower,
                            kPersonCurtains.upper,     kAllCurtains.lower,        kAllCurtains.upper};
    for (int tag : curtains) resetCurtainAction(this, tag);
}

// orig 001de6dc
void CanvasLayer::addCurtain(int tag)
{
    const CCSize size = getCanvasSize();
    CCLayerColor *curtain = CCLayerColor::create(ccc4(0, 0, 0, 255));
    curtain->runAction(CCSequence::create(CCHide::create(), NULL));
    // CCLayerColor ignores its anchor; shift it so it is centred on the canvas origin like everything else.
    curtain->setPosition(CCPoint(-size.width * 0.5f, -size.height * 0.5f));
    addChild(curtain, tag, tag);
}

// orig 001de5e0
void CanvasLayer::addDoubleCurtain(int topTag, int bottomTag)
{
    // Both hang from their top edge at the canvas origin; the first is flipped (scale -1) so its gradient faces up.
    HalfGradientLayer *flipped = HalfGradientLayer::create();
    flipped->ignoreAnchorPointForPosition(false);
    flipped->setAnchorPoint(CCPoint(0.5f, 1.0f));
    flipped->setPosition(0.0f, 0.0f);
    flipped->setScale(-1.0f);
    flipped->runAction(CCHide::create());
    addChild(flipped, topTag, topTag);

    HalfGradientLayer *upright = HalfGradientLayer::create();
    upright->ignoreAnchorPointForPosition(false);
    upright->setAnchorPoint(CCPoint(0.5f, 1.0f));
    upright->setPosition(0.0f, 0.0f);
    upright->setScale(1.0f);
    upright->runAction(CCHide::create());
    addChild(upright, bottomTag, bottomTag);
}

// orig 001de6b0
CCSize CanvasLayer::getCanvasSize()
{
    return CCDirector::sharedDirector()->getWinSize();
}

// orig 001de76c
void CanvasLayer::resetCanvasSize()
{
    setContentSize(getCanvasSize());
}

// orig 001de92a
void CanvasLayer::setDefaultPosition(const CCPoint &position)
{
    m_defaultPosition = position;
    m_currentPosition = m_defaultPosition;
    setPosition(m_defaultPosition);
    resetCanvasSize();
}

// orig 001de958
CCPoint CanvasLayer::getDefaultPosition()
{
    return m_defaultPosition;
}

// orig 001de900
void CanvasLayer::resetPosition()
{
    m_currentPosition = m_defaultPosition;
    setPosition(m_defaultPosition);
}

// orig 001de240
void CanvasLayer::onCameraAnimationFinished(CCObject *sender)
{
    setPosition(m_currentPosition);
}

// orig 001de922
void CanvasLayer::onShakeFinished(CCObject *sender)
{
    resetPosition();
}
