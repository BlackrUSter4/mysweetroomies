#ifndef SCENECONNECTOR_H
#define SCENECONNECTOR_H

#include <string>

// Scene switching requested by the Java side (jp/mfapps/common/jni/SceneConnector) and reported back to it.
//
// Flow:
//   1. Java calls SceneConnector.jni_startScene(name, callbackParameter) -> startScene(): the request is stored and,
//      if the running scene is a BeforeScene or an AfterScene (i.e. the engine is idle), a fresh BeforeScene
//      replaces it. Requests arriving while another scene runs are only stored.
//   2. BeforeScene::onEnterTransitionDidFinish() calls onScenePrepared(), which hands the stored scene to
//      BeforeScene::scheduleNextScene(AvailableSceneNames[name]) ("start_game", "adventure", "option_menu", "debug",
//      "notice"; an unknown name maps to value 0).
//   3. When the scene is done (AfterScene), onSceneEnd() calls Java SceneConnector.jni_onSceneEnd(name, parameter).
//
// UI code may also set the next scene directly (setSceneName / setCallbackParameter) before going through
// BeforeScene, as DebugSceneLayer and StartGameSceneLayer do.
class SceneConnector {
public:
    static SceneConnector* getInstance();
    virtual ~SceneConnector();
    bool init();  // clears both strings

    // Stores the request, then prepareScene().
    void startScene(const char* sceneName, const char* callbackParameter);
    // Replaces a running BeforeScene/AfterScene with a new BeforeScene (see above); otherwise does nothing.
    void prepareScene();
    // Called by BeforeScene once it is on screen: schedules the stored scene (if any).
    void onScenePrepared();

    // Java SceneConnector.jni_onSceneEnd(sceneName, callbackParameter).
    void onSceneEnd(const char* sceneName, const char* callbackParameter);
    void onSceneEnd();  // with the stored name and parameter

    void setSceneName(const char* sceneName);
    void setCallbackParameter(const char* callbackParameter);
    // Pointers into the stored strings; valid until the next set/start call.
    const char* getSceneName();
    const char* getCallbackParameter();

private:
    SceneConnector() {}
    // Stores name/parameter (null -> "") and schedules the scene on the running BeforeScene.
    void _startScene(const char* sceneName, const char* callbackParameter);

    std::string m_sceneName;
    std::string m_callbackParameter;
};

#endif
