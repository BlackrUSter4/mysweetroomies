#include "SceneConnector.h"

#include <jni.h>

#include "AfterScene.h"
#include "BeforeScene.h"
#include "ExEnums.h"
#include "cocos2d.h"
#include "platform/android/jni/JniHelper.h"

using namespace cocos2d;

namespace {
const char* CLASS_NAME = "jp/mfapps/common/jni/SceneConnector";
}  // namespace

// orig 001c07a4
SceneConnector* SceneConnector::getInstance() {
    static SceneConnector instance;
    static bool initialized = instance.init();
    (void)initialized;
    return &instance;
}

// orig 001c0780
SceneConnector::~SceneConnector() {}

// orig 001c581c
bool SceneConnector::init() {
    m_sceneName = "";
    m_callbackParameter = "";
    return true;
}

// orig 001c589c
const char* SceneConnector::getSceneName() {
    return m_sceneName.c_str();
}

// orig 001c58a0
const char* SceneConnector::getCallbackParameter() {
    return m_callbackParameter.c_str();
}

// orig 001c58a4
void SceneConnector::setSceneName(const char* sceneName) {
    m_sceneName = sceneName;
}

// orig 001c58b2
void SceneConnector::setCallbackParameter(const char* callbackParameter) {
    m_callbackParameter = callbackParameter;
}

// orig 001c58c0
void SceneConnector::prepareScene() {
    CCScene* running = CCDirector::sharedDirector()->getRunningScene();
    if (running == nullptr) {
        return;
    }
    if (dynamic_cast<BeforeScene*>(running) != nullptr || dynamic_cast<AfterScene*>(running) != nullptr) {
        CCDirector::sharedDirector()->replaceScene(BeforeScene::create());
    }
}

// orig 001c5918
void SceneConnector::startScene(const char* sceneName, const char* callbackParameter) {
    setSceneName(sceneName);
    setCallbackParameter(callbackParameter);
    prepareScene();
}

// orig 001c5934
void SceneConnector::onSceneEnd(const char* sceneName, const char* callbackParameter) {
    JniMethodInfo info;
    // The original calls without checking the lookup; skipping on failure avoids calling through a null method.
    if (!JniHelper::getStaticMethodInfo(info, CLASS_NAME, "jni_onSceneEnd",
                                        "(Ljava/lang/String;Ljava/lang/String;)V")) {
        return;
    }
    jstring jname = info.env->NewStringUTF(sceneName);
    jstring jparameter = info.env->NewStringUTF(callbackParameter);
    info.env->CallStaticVoidMethod(info.classID, info.methodID, jname, jparameter);
    info.env->DeleteLocalRef(info.classID);
    info.env->DeleteLocalRef(jname);
    info.env->DeleteLocalRef(jparameter);
}

// orig 001c59a8
void SceneConnector::onSceneEnd() {
    onSceneEnd(getSceneName(), getCallbackParameter());
}

// orig 001c5c5c
void SceneConnector::_startScene(const char* sceneName, const char* callbackParameter) {
    // Copy first: the callers pass pointers into m_sceneName / m_callbackParameter themselves.
    std::string name = sceneName ? sceneName : "";
    std::string parameter = callbackParameter ? callbackParameter : "";
    m_sceneName = name;
    m_callbackParameter = parameter;

    // The original dereferences the cast result unconditionally; it is only ever reached from BeforeScene.
    BeforeScene* before = dynamic_cast<BeforeScene*>(CCDirector::sharedDirector()->getRunningScene());
    if (before == nullptr) {
        return;
    }
    // map::operator[] in the original: an unknown name yields value 0.
    before->scheduleNextScene(AvailableSceneNames.toEnum(name, static_cast<AvailableScene>(0)));
}

// orig 001c5cec
void SceneConnector::onScenePrepared() {
    const char* name = getSceneName();
    const char* parameter = getCallbackParameter();
    if (name != nullptr) {
        _startScene(name, parameter);
    }
}
