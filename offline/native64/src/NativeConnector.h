#ifndef NATIVECONNECTOR_H
#define NATIVECONNECTOR_H

#include <string>

#include "LogMaker.h"

// Legacy native-side API surface of the original engine. Only the config accessors do anything; every other
// method is empty in the shipped library too (nothing in the game calls this class).
//
// The const char* getters return a pointer into a per-method buffer that stays valid until the same method is
// called again. (The original returned the buffer of a temporary std::string, which only worked by accident of
// libstdc++'s reference-counted strings.)
class NativeConnector {
public:
    static void Log(LOG level, std::string message);  // empty
    static void sendString(char* text);               // empty
    static void goMainWebView();                      // empty
    static void ResourceRequestSuccess(const char* path);  // empty
    static void ResourceRequestFail(const char* path);     // empty

    static void setStoryId(const char* storyId);       // AppConfigData story_id, committed
    static const char* getStoryId();
    static const char* getCurrentResourceUrl();        // AppConfigData::getResourceUrl()
    static const char* getRequestOffsetJson();         // always nullptr
    static const char* getRequestStoryJson(const char* storyId);  // always nullptr
    static void setEnvironment(const char* env);       // AppConfigData::switching(env), committed
    static const char* getCurrentEnvironment();        // AppConfigData::getEnv()
    static const char* getEnvironmentListJson();       // AppConfigData::getEnvironmentListJson()
    static const char* getString(const char* key, const char* defaultValue);  // AppConfigData::getString
    static void putString(const char* key, const char* value);               // AppConfigData::putString, committed

    static void launchCocos2dx();                    // empty
    static void startSplashScene();                  // empty
    static void startConversationScene();            // empty
    static void isLoadingScene();                    // empty
    static void prepareScene();                      // empty
    static void onApplicationDidFinishLaunching();   // empty
    static void onApplicationDidEnterForeground();   // empty
    static void onApplicationDidEnterBackground();   // empty
    static void onEnterBeforeScene();                // empty
    static void onEnterAfterScene();                 // empty
};

#endif
