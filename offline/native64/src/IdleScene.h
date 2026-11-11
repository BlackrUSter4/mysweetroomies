// IdleScene: an opaque black scene that hands over to an AfterScene as soon as it is on screen. The original exports
// no create() for it and nothing in the library instantiates it; kept for completeness.
#ifndef IDLESCENE_H
#define IDLESCENE_H

#include "cocos2d.h"

class IdleScene : public cocos2d::CCScene {
public:
    virtual ~IdleScene();
    virtual bool init();
    virtual void onEnterTransitionDidFinish();
};

#endif
