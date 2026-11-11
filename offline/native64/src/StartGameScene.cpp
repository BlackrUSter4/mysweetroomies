#include "StartGameScene.h"

#include <string>

#include "AppConfigData.h"
#include "SoundEngine.h"

USING_NS_CC;

// orig 001caeb4
StartGameScene* StartGameScene::create()
{
    StartGameScene* scene = new StartGameScene();
    if (!scene->init()) {
        delete scene;
        return nullptr;
    }
    scene->autorelease();
    return scene;
}

// orig 001cad8c
StartGameScene::~StartGameScene() {}

// orig 001cafd0
void StartGameScene::onEnterTransitionDidFinish()
{
    CCScene::onEnterTransitionDidFinish();
    // Unlike OptionMenuScene's, this save callback does not commit: volume changes made on the title screen reach
    // the disk with the next commit of AppConfigData (as in the original).
    SoundEngine::getInstance()->prepare(
        [](const std::string& key, float defaultValue) -> float {
            return static_cast<float>(AppConfigData::getInstance()->getDouble(key, defaultValue));
        },
        [](const std::string& key, float value) { AppConfigData::getInstance()->putDouble(key, value); });
    SoundEngine::getInstance()->playBgm("Application/sound/bgm/splash/13_01.ogg");
}

// orig 001cb080
void StartGameScene::onExit()
{
    CCScene::onExit();
    SoundEngine::getInstance()->release();
}
