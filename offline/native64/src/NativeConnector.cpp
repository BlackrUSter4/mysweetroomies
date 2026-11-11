#include "NativeConnector.h"

#include "AppConfigData.h"

namespace {
const char* keep(std::string& buffer, const std::string& value) {
    buffer = value;
    return buffer.c_str();
}
}  // namespace

// orig 001c5f4c .. 001c5f54: empty in the original.
void NativeConnector::Log(LOG level, std::string message) {
    (void)level;
    (void)message;
}
void NativeConnector::sendString(char* text) { (void)text; }
void NativeConnector::goMainWebView() {}
void NativeConnector::ResourceRequestSuccess(const char* path) { (void)path; }
void NativeConnector::ResourceRequestFail(const char* path) { (void)path; }

// orig 001c5f56
void NativeConnector::setStoryId(const char* storyId) {
    AppConfigData::getInstance()->setStoryId(std::string(storyId)).commit();
}

// orig 001c5f9e
const char* NativeConnector::getStoryId() {
    static std::string buffer;
    return keep(buffer, AppConfigData::getInstance()->getStoryId());
}

// orig 001c5fba
const char* NativeConnector::getCurrentResourceUrl() {
    static std::string buffer;
    return keep(buffer, AppConfigData::getInstance()->getResourceUrl());
}

// orig 001c5fd6
const char* NativeConnector::getRequestOffsetJson() {
    return nullptr;
}

// orig 001c5fda
const char* NativeConnector::getRequestStoryJson(const char* storyId) {
    (void)storyId;
    return nullptr;
}

// orig 001c5fde
void NativeConnector::setEnvironment(const char* env) {
    AppConfigData::getInstance()->switching(std::string(env)).commit();
}

// orig 001c600e
const char* NativeConnector::getCurrentEnvironment() {
    static std::string buffer;
    return keep(buffer, AppConfigData::getInstance()->getEnv());
}

// orig 001c602c
const char* NativeConnector::getEnvironmentListJson() {
    static std::string buffer;
    return keep(buffer, AppConfigData::getInstance()->getEnvironmentListJson());
}

// orig 001c6048
const char* NativeConnector::getString(const char* key, const char* defaultValue) {
    static std::string buffer;
    return keep(buffer, AppConfigData::getInstance()->getString(key, defaultValue));
}

// orig 001c60ae
void NativeConnector::putString(const char* key, const char* value) {
    AppConfigData::getInstance()->putString(key, value).commit();
}

// orig 001c610a .. 001c611c: empty in the original.
void NativeConnector::launchCocos2dx() {}
void NativeConnector::startSplashScene() {}
void NativeConnector::startConversationScene() {}
void NativeConnector::isLoadingScene() {}
void NativeConnector::prepareScene() {}
void NativeConnector::onApplicationDidFinishLaunching() {}
void NativeConnector::onApplicationDidEnterForeground() {}
void NativeConnector::onApplicationDidEnterBackground() {}
void NativeConnector::onEnterBeforeScene() {}
void NativeConnector::onEnterAfterScene() {}
