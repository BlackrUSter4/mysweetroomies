// OptionsDialog: the settings window (title screen, OptionMenuScene, in-story overlay menu). Three sliders,
// "reset" and a close button on the bg_setting.png frame, all from Framework/ui/menu_option.plist:
//
//   BGM volume   (tag 0)  -> SoundEngine::setVolumeBgm; releasing the thumb plays a BGM sample
//   SE / voice   (tag 1)  -> SoundEngine::setVolumeSe; releasing the thumb plays a voice sample
//   text speed   (tag 3)  -> SceneManager::setAutoPlayDelay (slider right = faster = shorter delay)
//   reset        (tag 4, a CCMenu) puts all three back to their defaults
//
// Taps outside the frame do not close it. Closing stops the samples (SoundEngine::stop()) before MiModalLayer
// runs the dismiss callback.
#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <functional>

#include "GUI/CCControlExtension/CCControlSlider.h"
#include "MiModalLayer.h"
#include "cocos2d.h"

// Child tags of the dialog (also the setDefaultValue() argument). Fixed underlying type so UiHelpers.h can
// declare it opaquely.
enum OptionsDialogTag : int {
    OptionsDialogTag_Bgm = 0,
    OptionsDialogTag_Se = 1,
    OptionsDialogTag_TextSpeed = 3,
    OptionsDialogTag_Reset = 4,
};

class OptionsDialog : public MiModalLayer {
public:
    static OptionsDialog* create();
    static cocos2d::CCScene* scene();  // a scene holding only the dialog
    virtual ~OptionsDialog();
    virtual bool init();   // MiModalLayer::init, then initUi()
    virtual void initUi();
    virtual void dismiss();  // SoundEngine::stop(), then MiModalLayer::dismiss()

    // Adds the label sprite `labelFrame` at height y (window centre x) and a CCControlSliderTouchable just below
    // it, child `tag`, range min..max starting at value, reporting value changes to `selector` and the end of a
    // drag to `onTouchEnd`.
    void buildSlider(cocos2d::extension::SEL_CCControlHandler selector, float y, int tag, float min, float max,
                     float value, const char* labelFrame, std::function<void()> onTouchEnd);

    void onBgmChanged(cocos2d::extension::CCControlSlider* slider);
    void onSeChanged(cocos2d::extension::CCControlSlider* slider);
    void onAutoTextChanged(cocos2d::extension::CCControlSlider* slider);
    void onDefaultButtonClicked(cocos2d::CCObject* sender);
    void onCloseButtonClicked(cocos2d::CCObject* sender);  // dismiss()

private:
    // CCControl targets with the SEL_CCControlHandler signature; the original registered the one-argument
    // handlers above through a cast.
    void onBgmSliderEvent(cocos2d::CCObject* sender, cocos2d::extension::CCControlEvent event);
    void onSeSliderEvent(cocos2d::CCObject* sender, cocos2d::extension::CCControlEvent event);
    void onTextSpeedSliderEvent(cocos2d::CCObject* sender, cocos2d::extension::CCControlEvent event);
};

#endif
