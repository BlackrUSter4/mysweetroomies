#include "OptionMenuScene.h"

#include <string>

#include "AfterScene.h"
#include "AppConfigData.h"
#include "OptionsDialog.h"
#include "SoundEngine.h"

USING_NS_CC;

// orig 001c84c4
OptionMenuScene* OptionMenuScene::create()
{
    OptionMenuScene* scene = new OptionMenuScene();
    if (!scene->init()) {
        delete scene;
        return nullptr;
    }
    scene->autorelease();
    return scene;
}

// orig 001ca8bc
OptionMenuScene::~OptionMenuScene() {}

// orig 001cab28
bool OptionMenuScene::init()
{
    if (!CCScene::init()) return false;
    addChild(CCLayerColor::create(ccc4(0xee, 0xee, 0xee, 0xff)));
    OptionsDialog* dialog = OptionsDialog::create();
    addChild(dialog);
    // Closing the dialog is the end of this scene; Java decides what comes next.
    dialog->setDismissCallback([] { CCDirector::sharedDirector()->replaceScene(AfterScene::create()); });
    return true;
}

// orig 001ca9b0
void OptionMenuScene::onEnterTransitionDidFinish()
{
    // Deviation: the original skipped the base call (and the one in onExit), so its children never got their
    // enter-finished / exit notifications. Calling it keeps CCNode's bookkeeping (running flag, scheduler,
    // touch registration of the dialog) consistent.
    CCScene::onEnterTransitionDidFinish();
    SoundEngine::getInstance()->prepare(
        [](const std::string& key, float defaultValue) -> float {
            return static_cast<float>(AppConfigData::getInstance()->getDouble(key, defaultValue));
        },
        [](const std::string& key, float value) { AppConfigData::getInstance()->putDouble(key, value).commit(); });
}

// orig 001caa4c
void OptionMenuScene::onExit()
{
    CCScene::onExit();
    SoundEngine::getInstance()->release();
}
