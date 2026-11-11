#ifndef APPDELEGATE_H
#define APPDELEGATE_H

#include "cocos2d.h"

// The cocos2d-x application object, created by Cocos2dxRenderer.nativeInit (main.cpp) on first GL surface.
//
// Launch: display stats off, 60 fps, run a BeforeScene (the idle scene that waits for Java's
// SceneConnector.jni_startScene), then give SoundEngine and SceneManager their AppConfigData-backed float
// persistence (getDouble / putDouble + commit). Background/foreground pause and resume the director and all audio.
class AppDelegate : private cocos2d::CCApplication {
public:
    AppDelegate();
    virtual ~AppDelegate();

    virtual bool applicationDidFinishLaunching();
    virtual void applicationDidEnterBackground();
    virtual void applicationWillEnterForeground();
};

#endif
