#include "AppDelegate.h"

#include <string>

#include "AppConfigData.h"
#include "BeforeScene.h"
#include "SceneManager.h"
#include "SoundEngine.h"

using namespace cocos2d;

namespace {
// orig 001c7020 / 001c706e: the reader both SoundEngine and SceneManager get.
float loadConfigFloat(const std::string& key, float defaultValue) {
    return static_cast<float>(AppConfigData::getInstance()->getDouble(key, static_cast<double>(defaultValue)));
}

// orig 001c7048 / 001c7096: the writer; every change is persisted immediately.
void saveConfigFloat(const std::string& key, float value) {
    AppConfigData::getInstance()->putDouble(key, static_cast<double>(value)).commit();
}
}  // namespace

// orig 001c70bc
AppDelegate::AppDelegate() {}

// orig 001c6df0
AppDelegate::~AppDelegate() {}

// orig 001c7340
bool AppDelegate::applicationDidFinishLaunching() {
    CCDirector* director = CCDirector::sharedDirector();
    director->setOpenGLView(CCEGLView::sharedOpenGLView());
    director->setDisplayStats(false);
    director->setAnimationInterval(1.0 / 60);
    director->runWithScene(BeforeScene::create());

    SoundEngine::getInstance()->prepare(loadConfigFloat, saveConfigFloat);
    SceneManager::getInstance()->setIO(loadConfigFloat, saveConfigFloat);
    return true;
}

// orig 001c6ff4
void AppDelegate::applicationDidEnterBackground() {
    CCDirector::sharedDirector()->stopAnimation();
    SoundEngine::getInstance()->pause();
}

// orig 001c700a
void AppDelegate::applicationWillEnterForeground() {
    CCDirector::sharedDirector()->startAnimation();
    SoundEngine::getInstance()->resume();
}
