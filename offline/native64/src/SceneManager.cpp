#include "SceneManager.h"

#include "ExOffsetData.h"
#include "ExStoryData.h"
#include "ResourceManager.h"
#include "SceneDataParser.h"

USING_NS_CC;

const float SceneManager::AUTOPLAY_DELAY = 4.25f;
const int SceneManager::ON_LINK_SCENE_ACTION_TAG = 10;

namespace {
const char *const kAutoPlayDelayKey = "scene_manager.text_speed";
const char *const kAutoPlayKey = "scene_manager.is_auto_play";
// Pause between a link_scene line finishing and the next one starting.
const float kLinkSceneDelay = 0.25f;
}  // namespace

SceneManager *SceneManager::getInstance()
{
    static SceneManager instance;
    return &instance;
}

SceneManager::SceneManager()
{
    init();
}

SceneManager::~SceneManager() {}

bool SceneManager::init()
{
    m_autoPlay = false;
    m_paused = false;
    m_currentIndex = -1;
    m_autoPlayDelay = AUTOPLAY_DELAY;
    resetOnStoryFinishListener();
    setDefaultIO();
    return true;
}

// --- story --------------------------------------------------------------------------------------------------------

void SceneManager::start(const std::string &offsetJsonPath, const std::string &storyJsonPath)
{
    prepare(offsetJsonPath, storyJsonPath);
    startWithoutPrepare();
}

void SceneManager::prepare(const std::string &offsetJsonPath, const std::string &storyJsonPath)
{
    prepareData(offsetJsonPath, storyJsonPath);
}

void SceneManager::prepareData(const std::string &offsetJsonPath, const std::string &storyJsonPath)
{
    std::string offsetJson = ResourceManager::getInstance()->slurp(offsetJsonPath);
    std::shared_ptr<ExOffsetData> offsets = ExOffsetData::parse(offsetJson.c_str());
    std::string storyJson = ResourceManager::getInstance()->slurp(storyJsonPath);
    std::shared_ptr<ExStoryData> story = ExStoryData::parse(storyJson.c_str());
    prepareData(SceneDataParser::parse(story, offsets));
}

void SceneManager::prepareData(std::vector<std::shared_ptr<SceneData>> scenes)
{
    m_scenes = scenes;
    m_currentIndex = -1;
}

void SceneManager::startWithoutPrepare()
{
    startListenerEvent(SceneEventType_Setup);
    load();
    if (isAutoPlay()) startAutoPlayTimer();
}

void SceneManager::next()
{
    if (isPaused() || isSceneLocking()) return;
    if (m_currentIndex < (int)m_scenes.size() - 1) {
        ++m_currentIndex;
        startListenerEvent(m_currentIndex == 0 ? SceneEventType_PreRender : SceneEventType_PostEffect);
    } else {
        finish();
    }
}

void SceneManager::nextWithDelay(float seconds)
{
    // The original cast onLinkSceneCallBack(CCObject*) to a plain CCCallFunc selector; CCCallFuncO with a null
    // object makes the same call without relying on that cast.
    CCSequence *action = CCSequence::create(
        CCDelayTime::create(seconds),
        CCCallFuncO::create(this, callfuncO_selector(SceneManager::onLinkSceneCallBack), NULL), NULL);
    action->setTag(ON_LINK_SCENE_ACTION_TAG);
    CCScene *running = CCDirector::sharedDirector()->getRunningScene();
    if (running) running->runAction(action);  // the original assumed a running scene
}

void SceneManager::stopNextWithDelay()
{
    CCScene *running = CCDirector::sharedDirector()->getRunningScene();
    if (running) running->stopActionByTag(ON_LINK_SCENE_ACTION_TAG);
}

void SceneManager::finish()
{
    startListenerEvent(SceneEventType_Finish);
}

bool SceneManager::hasSceneData()
{
    return !m_scenes.empty();
}

std::shared_ptr<SceneData> SceneManager::getCurrentSceneData()
{
    // The original used vector::at and threw before the first next(); null is returned instead.
    if (!hasSceneData() || m_currentIndex < 0 || m_currentIndex >= (int)m_scenes.size())
        return std::shared_ptr<SceneData>();
    return m_scenes[m_currentIndex];
}

bool SceneManager::isFinished()
{
    if (m_listeners.empty()) return true;
    return m_currentIndex >= (int)m_scenes.size() - 1;
}

// --- listeners ----------------------------------------------------------------------------------------------------

void SceneManager::addListener(const std::string &name, std::shared_ptr<SceneEventListener> listener)
{
    m_listeners.insert(std::make_pair(name, listener));
    for (std::map<SceneEventType, std::string>::const_iterator it = SceneEventTypeNames.enumToString.begin();
         it != SceneEventTypeNames.enumToString.end(); ++it)
        m_doneFlags.insert(std::make_pair(SceneListenerEventKey(name, it->first), 0));
}

void SceneManager::removeListener(const std::string &name)
{
    m_listeners.erase(name);
    for (std::map<SceneListenerEventKey, int>::iterator it = m_doneFlags.begin(); it != m_doneFlags.end();) {
        if (it->first.name == name)
            m_doneFlags.erase(it++);
        else
            ++it;
    }
}

void SceneManager::clearListener()
{
    m_listeners.clear();
    m_doneFlags.clear();
    removeAllAutoPlayChangedListener();
}

void SceneManager::startListenerEvent(SceneEventType event)
{
    m_currentEvent = event;

    // Callbacks may report done synchronously, which can start the next phase (or remove listeners) from inside this
    // loop; iterating a snapshot keeps that safe. The original walked the live map.
    std::vector<std::shared_ptr<SceneEventListener>> listeners;
    for (std::map<std::string, std::shared_ptr<SceneEventListener>>::iterator it = m_listeners.begin();
         it != m_listeners.end(); ++it)
        listeners.push_back(it->second);

    if (event == SceneEventType_Setup) {
        for (size_t i = 0; i < listeners.size(); ++i) listeners[i]->onSetup(m_scenes);
        return;
    }

    // The original used vector::at here and threw for an empty story; null is passed instead.
    std::shared_ptr<SceneData> current = getCurrentSceneData();
    std::shared_ptr<SceneData> previous;
    if (m_currentIndex >= 1 && m_currentIndex - 1 < (int)m_scenes.size()) previous = m_scenes[m_currentIndex - 1];

    for (std::map<SceneListenerEventKey, int>::iterator it = m_doneFlags.begin(); it != m_doneFlags.end(); ++it) {
        if (it->first.type == event) it->second = 0;
    }

    switch (event) {
    case SceneEventType_MainRender:
        for (size_t i = 0; i < listeners.size(); ++i) listeners[i]->onMainRender(current);
        break;
    case SceneEventType_PostEffect:
        for (size_t i = 0; i < listeners.size(); ++i) listeners[i]->onPostEffect(previous);
        break;
    case SceneEventType_PreRender:
        for (size_t i = 0; i < listeners.size(); ++i) listeners[i]->onPreRender(previous, current);
        break;
    case SceneEventType_PreEffect:
        for (size_t i = 0; i < listeners.size(); ++i) listeners[i]->onPreEffect(previous, current);
        break;
    case SceneEventType_Finish:
        for (size_t i = 0; i < listeners.size(); ++i) listeners[i]->onFinishScene();
        clearListener();
        break;
    default:
        break;
    }
}

void SceneManager::doneListenerEvent(SceneEventType event, const std::string &name)
{
    if (event != m_currentEvent) return;

    m_doneFlags[SceneListenerEventKey(name, event)] = 1;
    int expected = 0, done = 0;
    for (std::map<SceneListenerEventKey, int>::iterator it = m_doneFlags.begin(); it != m_doneFlags.end(); ++it) {
        if (it->first.type != event) continue;
        ++expected;
        done += it->second;
    }
    if (done != expected) return;

    stopNextWithDelay();
    switch (event) {
    case SceneEventType_Setup:
        next();
        break;
    case SceneEventType_MainRender: {
        std::shared_ptr<SceneData> current = getCurrentSceneData();
        if (current && current->text && current->text->linkScene == ExSceneLinkScene_Link)
            nextWithDelay(kLinkSceneDelay);
        break;
    }
    case SceneEventType_PostEffect:
        startListenerEvent(SceneEventType_PreRender);
        break;
    case SceneEventType_PreRender:
        startListenerEvent(SceneEventType_PreEffect);
        break;
    case SceneEventType_PreEffect:
        startListenerEvent(SceneEventType_MainRender);
        break;
    case SceneEventType_Finish:
        onScenePlayFinished();
        break;
    }
}

void SceneManager::setOnStoryFinishListener(std::function<void()> listener)
{
    m_onStoryFinish = listener;
}

void SceneManager::resetOnStoryFinishListener()
{
    m_onStoryFinish = []() {};
}

void SceneManager::onScenePlayFinished()
{
    if (isAutoPlay()) stopAutoPlayTimer();
    // Called through a copy: the callback may install a new listener while it runs.
    std::function<void()> onFinish = m_onStoryFinish;
    if (onFinish) onFinish();
    resetOnStoryFinishListener();
}

// --- pause / locks ------------------------------------------------------------------------------------------------

void SceneManager::pause()
{
    m_paused = true;
}

void SceneManager::resume()
{
    m_paused = false;
}

bool SceneManager::isPaused()
{
    return m_paused;
}

void SceneManager::lockScene(const std::string &owner)
{
    m_locks[owner] = true;
}

void SceneManager::unlockScene(const std::string &owner)
{
    m_locks[owner] = false;
    m_locks.erase(owner);
}

bool SceneManager::isSceneLocking()
{
    for (std::map<std::string, bool>::iterator it = m_locks.begin(); it != m_locks.end(); ++it) {
        if (it->second) return true;
    }
    return false;
}

// --- auto-play ----------------------------------------------------------------------------------------------------

bool SceneManager::isAutoPlay()
{
    return m_autoPlay;
}

void SceneManager::setAutoPlay(bool enabled)
{
    if (m_autoPlay != enabled) {
        if (enabled)
            startAutoPlayTimer();
        else
            stopAutoPlayTimer();
    }
    m_autoPlay = enabled;
    saveAutoPlay(enabled);
    notifyAutoPlayChangedListener(enabled, getCurrentSceneData());
}

void SceneManager::saveAutoPlay(bool enabled)
{
    saveValue(kAutoPlayKey, enabled ? 1.0f : 0.0f);
}

float SceneManager::getAutoPlayDelay()
{
    m_autoPlayDelay = m_loadValue(kAutoPlayDelayKey, m_autoPlayDelay);
    return m_autoPlayDelay;
}

void SceneManager::setAutoPlayDelay(float seconds)
{
    m_autoPlayDelay = seconds;
    if (isAutoPlay() && hasSceneData()) restartAutoPlayTimer();
    saveValue(kAutoPlayDelayKey, m_autoPlayDelay);
}

void SceneManager::addAutoPlayChangedListener(AutoPlayChangedListener listener, const std::string &name)
{
    m_autoPlayListeners[name] = listener;
}

void SceneManager::removeAutoPlayChangedListener(const std::string &name)
{
    m_autoPlayListeners.erase(name);
}

void SceneManager::removeAllAutoPlayChangedListener()
{
    m_autoPlayListeners.clear();
}

void SceneManager::notifyAutoPlayChangedListener(bool enabled, std::shared_ptr<SceneData> current)
{
    for (std::map<std::string, AutoPlayChangedListener>::iterator it = m_autoPlayListeners.begin();
         it != m_autoPlayListeners.end(); ++it) {
        AutoPlayChangedListener listener = it->second;
        listener(enabled, current);
    }
}

void SceneManager::startAutoPlayTimer()
{
    CCDirector::sharedDirector()->getScheduler()->scheduleSelector(
        schedule_selector(SceneManager::updateSchedule), this, m_autoPlayDelay, false);
}

void SceneManager::stopAutoPlayTimer()
{
    CCDirector::sharedDirector()->getScheduler()->unscheduleSelector(schedule_selector(SceneManager::updateSchedule),
                                                                     this);
}

void SceneManager::restartAutoPlayTimer()
{
    stopAutoPlayTimer();
    startAutoPlayTimer();
}

void SceneManager::updateSchedule(float)
{
    next();
}

void SceneManager::onLinkSceneCallBack(CCObject *)
{
    updateSchedule(0.0f);
}

// --- persistence --------------------------------------------------------------------------------------------------

void SceneManager::setIO(LoadFunction load, SaveFunction save)
{
    m_loadValue = load;
    m_saveValue = save;
    this->load();
}

void SceneManager::setDefaultIO()
{
    m_loadValue = [](const std::string &, float defaultValue) -> const float { return defaultValue; };
    m_saveValue = [](const std::string &, float) {};
}

void SceneManager::load()
{
    m_autoPlayDelay = loadValue(kAutoPlayDelayKey, AUTOPLAY_DELAY);
    m_autoPlay = loadValue(kAutoPlayKey, 0.0f) == 1.0f;
}

float SceneManager::loadValue(const std::string &key, float defaultValue)
{
    return m_loadValue(key, defaultValue);
}

void SceneManager::saveValue(const std::string &key, float value)
{
    m_saveValue(key, value);
}
