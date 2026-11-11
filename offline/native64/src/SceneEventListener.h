// SceneEventListener: the six callbacks a subsystem registers with SceneManager::addListener, one per SceneEventType.
// SceneListenerEventKey: (listener name, event) - the key of SceneManager's "has this listener finished this phase"
// table.
//
//   auto listener = std::make_shared<SceneEventListener>(
//       [this](const std::vector<std::shared_ptr<SceneData>> &all) { ...; done(SceneEventType_Setup); },
//       [this](std::shared_ptr<SceneData> current)                { ...; done(SceneEventType_MainRender); },
//       [this](std::shared_ptr<SceneData> prev, std::shared_ptr<SceneData> cur) { ...PreRender... },
//       [this](std::shared_ptr<SceneData> prev, std::shared_ptr<SceneData> cur) { ...PreEffect... },
//       [this](std::shared_ptr<SceneData> prev)                    { ...PostEffect... },
//       [this]()                                                   { ...FinishScene... });
//   SceneManager::getInstance()->addListener("CanvasLayer", listener);
// where done(e) is SceneManager::getInstance()->doneListenerEvent(e, "CanvasLayer"). Every callback must eventually
// report done for its event (immediately or when an animation ends), otherwise the story stalls. See SceneManager.h.
#ifndef SCENEEVENTLISTENER_H
#define SCENEEVENTLISTENER_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ExEnums.h"
#include "SceneData.h"

struct SceneEventListener {
    typedef std::function<void(const std::vector<std::shared_ptr<SceneData>> &)> SetupHandler;
    typedef std::function<void(std::shared_ptr<SceneData>)> SceneHandler;
    typedef std::function<void(std::shared_ptr<SceneData>, std::shared_ptr<SceneData>)> TransitionHandler;
    typedef std::function<void()> FinishHandler;

    // Argument order is the original's make_shared<SceneEventListener>(...) order.
    SceneEventListener(SetupHandler setup, SceneHandler mainRender, TransitionHandler preRender,
                       TransitionHandler preEffect, SceneHandler postEffect, FinishHandler finishScene)
        : onSetup(setup), onMainRender(mainRender), onPreRender(preRender), onPreEffect(preEffect),
          onPostEffect(postEffect), onFinishScene(finishScene) {}

    SetupHandler onSetup;            // SceneEventType_Setup: every scene of the story, before the first one
    SceneHandler onMainRender;       // SceneEventType_MainRender: the current scene
    TransitionHandler onPreRender;   // SceneEventType_PreRender: (previous or null, current)
    TransitionHandler onPreEffect;   // SceneEventType_PreEffect: (previous or null, current)
    SceneHandler onPostEffect;       // SceneEventType_PostEffect: the scene being left (never null)
    FinishHandler onFinishScene;     // SceneEventType_Finish: the story is over
};

struct SceneListenerEventKey {
    std::string name;     // listener name given to addListener
    SceneEventType type;  // phase

    SceneListenerEventKey(const std::string &listenerName, SceneEventType event) : name(listenerName), type(event) {}
};

// Name first, then event. orig 001cdc58
inline bool operator<(const SceneListenerEventKey &a, const SceneListenerEventKey &b)
{
    if (a.name < b.name) return true;
    return a.name == b.name && a.type < b.type;
}

#endif
