#ifndef MITOAST_H
#define MITOAST_H

#include <string>

#include "cocos2d.h"

// Short-lived message box centred on the running scene (debug/warning messages from the renderers).
// Child tags: 0 = the CCLabelTTF, 1 = the CCLayerColor background.
class MiToast : public cocos2d::CCLayer {
public:
    static const float DURATION_LONG;   // 10 s
    static const float DURATION_SHORT;  // 2 s

    static MiToast* create();  // also adds the toast to the running scene
    virtual ~MiToast();
    virtual bool init();

    // Shows `text` for `duration` seconds; textColor tints the label, backgroundColor the box.
    static void show(const std::string& text, float duration, cocos2d::ccColor4B textColor,
                     cocos2d::ccColor4B backgroundColor);
    static void debug(const std::string& text, float duration);
    static void info(const std::string& text, float duration);
    static void warn(const std::string& text, float duration);
    static void error(const std::string& text, float duration);

    void dismiss();  // removeFromParentAndCleanup(true)

private:
    void onDurationElapsed(float dt);
};

#endif
