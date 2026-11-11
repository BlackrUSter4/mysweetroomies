// SceneManager: plays a story. Owns the SceneData list, moves through it, and drives every registered
// SceneEventListener through the same phases for each scene, waiting until all of them report a phase done before
// starting the next one. Also owns auto-play (advance every getAutoPlayDelay() seconds) and scene locks.
//
// Lifecycle
//   start(offsetJsonPath, storyJsonPath)    = prepare(...) + startWithoutPrepare()
//     prepare -> prepareData: slurps both files through ResourceManager, parses ExOffsetData + ExStoryData,
//                SceneDataParser::parse, index = -1.
//     startWithoutPrepare: Setup phase (onSetup(all scenes)), load() saved settings, start the auto-play timer if
//                auto-play is on.
//
// Phase sequence (SceneEventType; each arrow waits for every listener's doneListenerEvent(phase, name)):
//   Setup -> next()
//   next(): ignored while paused or any lock is held; otherwise index++ and
//           first scene:  PreRender(null, scene0)
//           later scenes: PostEffect(previous) -> PreRender(previous, current)
//   PreRender -> PreEffect(previous, current) -> MainRender(current) -> [wait]
//   After MainRender: if current->text->linkScene == Link, next() runs 0.25 s later by itself; otherwise the player
//   (TouchLayer / text window -> next()) or the auto-play timer advances.
//   next() past the last scene -> Finish: every onFinishScene(), then all listeners are removed; when they all report
//   Finish done -> onScenePlayFinished(): auto-play timer stops, the story-finish callback runs once and is reset.
//
// Reporting done: call SceneManager::getInstance()->doneListenerEvent(phase, listenerName) when the work for that
// phase is finished (synchronously from the callback is fine). Reports for any phase other than the current one are
// ignored, so a late report from an earlier phase is harmless. A listener that never reports stalls the story.
//
// Persistence: auto-play state and delay go through setIO's load/save functions under the keys
// "scene_manager.is_auto_play" (1.0 = on) and "scene_manager.text_speed" (delay in seconds). The default IO keeps
// nothing (load returns the default, save does nothing); AppDelegate installs preference-backed functions.
#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ExEnums.h"
#include "SceneData.h"
#include "SceneEventListener.h"
#include "cocos2d.h"

class SceneManager : public cocos2d::CCObject {
public:
    typedef std::function<void(bool, std::shared_ptr<SceneData>)> AutoPlayChangedListener;
    typedef std::function<const float(const std::string &, float)> LoadFunction;  // (key, default) -> value
    typedef std::function<void(const std::string &, float)> SaveFunction;         // (key, value)

    static const float AUTOPLAY_DELAY;        // 4.25 s: default auto-play delay
    static const int ON_LINK_SCENE_ACTION_TAG;  // 10: tag of the delayed next() action on the running scene

    static SceneManager *getInstance();  // process-wide instance, created on first use

    SceneManager();
    virtual ~SceneManager();
    bool init();  // resets state, default IO, empty story-finish callback; called by the constructor

    // --- story ---
    void start(const std::string &offsetJsonPath, const std::string &storyJsonPath);
    void prepare(const std::string &offsetJsonPath, const std::string &storyJsonPath);
    void prepareData(const std::string &offsetJsonPath, const std::string &storyJsonPath);
    void prepareData(std::vector<std::shared_ptr<SceneData>> scenes);  // replaces the story, index = -1
    void startWithoutPrepare();
    void next();             // advance one scene (see above); finish() after the last one
    void nextWithDelay(float seconds);  // next() after `seconds` via an action on the running cocos2d scene
    void stopNextWithDelay();
    void finish();           // start the Finish phase now
    bool hasSceneData();
    // Scene at the current index, or null before the first next() / without a story.
    std::shared_ptr<SceneData> getCurrentSceneData();
    // True when no listener is registered or the current scene is the last one.
    bool isFinished();

    // --- listeners ---
    // Registers `listener` under `name` (an existing name keeps its old listener) and a done-flag for every phase.
    void addListener(const std::string &name, std::shared_ptr<SceneEventListener> listener);
    void removeListener(const std::string &name);
    void clearListener();  // removes every listener, done-flag and auto-play listener
    void startListenerEvent(SceneEventType event);
    void doneListenerEvent(SceneEventType event, const std::string &name);

    // Called once when the story's Finish phase completes, then reset to a no-op.
    void setOnStoryFinishListener(std::function<void()> listener);
    void resetOnStoryFinishListener();
    void onScenePlayFinished();

    // --- pause / locks ---
    // Paused or locked: next() does nothing (the auto-play timer keeps ticking but has no effect).
    void pause();
    void resume();
    bool isPaused();
    void lockScene(const std::string &owner);    // e.g. EffectRender while an end mark plays
    void unlockScene(const std::string &owner);
    bool isSceneLocking();

    // --- auto-play ---
    bool isAutoPlay();
    void setAutoPlay(bool enabled);  // starts/stops the timer, saves, notifies auto-play listeners
    void saveAutoPlay(bool enabled);
    float getAutoPlayDelay();        // reloads the delay through the load function and returns it (seconds)
    void setAutoPlayDelay(float seconds);  // restarts a running timer, saves
    void addAutoPlayChangedListener(AutoPlayChangedListener listener, const std::string &name);  // replaces `name`
    void removeAutoPlayChangedListener(const std::string &name);
    void removeAllAutoPlayChangedListener();
    void notifyAutoPlayChangedListener(bool enabled, std::shared_ptr<SceneData> current);
    virtual void startAutoPlayTimer();    // schedules updateSchedule every getAutoPlayDelay() seconds
    virtual void stopAutoPlayTimer();
    virtual void restartAutoPlayTimer();  // stop + start, picks up a new delay
    void updateSchedule(float dt);        // auto-play tick: next()
    void onLinkSceneCallBack(cocos2d::CCObject *sender);  // nextWithDelay's callback: next()

    // --- persistence ---
    void setIO(LoadFunction load, SaveFunction save);  // installs both functions, then load()
    void setDefaultIO();
    void load();  // reads auto-play delay and state
    float loadValue(const std::string &key, float defaultValue);
    void saveValue(const std::string &key, float value);

private:
    std::vector<std::shared_ptr<SceneData>> m_scenes;
    int m_currentIndex = -1;
    bool m_paused = false;
    bool m_autoPlay = false;
    float m_autoPlayDelay = AUTOPLAY_DELAY;
    SceneEventType m_currentEvent = SceneEventType_Setup;
    std::map<std::string, std::shared_ptr<SceneEventListener>> m_listeners;
    std::map<SceneListenerEventKey, int> m_doneFlags;  // 1 = listener finished that phase
    std::map<std::string, bool> m_locks;
    std::map<std::string, AutoPlayChangedListener> m_autoPlayListeners;
    std::function<void()> m_onStoryFinish;
    LoadFunction m_loadValue;
    SaveFunction m_saveValue;
};

#endif
