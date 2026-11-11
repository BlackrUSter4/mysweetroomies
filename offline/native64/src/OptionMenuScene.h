// OptionMenuScene: the stand-alone settings screen Java can open (AvailableScene "option_menu"): a light grey
// backdrop with an OptionsDialog; closing the dialog ends the scene through AfterScene.
#ifndef OPTIONMENUSCENE_H
#define OPTIONMENUSCENE_H

#include "cocos2d.h"

class OptionMenuScene : public cocos2d::CCScene {
public:
    static OptionMenuScene* create();
    virtual ~OptionMenuScene();
    virtual bool init();

    // SoundEngine::prepare() with AppConfigData-backed volume IO (committed on every change).
    virtual void onEnterTransitionDidFinish();
    virtual void onExit();  // SoundEngine::release()
};

#endif
