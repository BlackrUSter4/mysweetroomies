#ifndef MIMODALLAYER_H
#define MIMODALLAYER_H

#include <functional>

#include "cocos2d.h"

// Full-screen layer that swallows every touch below it (OptionsDialog, TextLogDialog, OverlayMenuLayer and MiDialog
// derive from it). A tap anywhere closes it unless setEnableCloseOnTouchLayer(false).
class MiModalLayer : public cocos2d::CCLayer {
public:
    // Touch priority of the modal layer itself; children that must stay clickable register below it
    // (kModalLayerPriority - 1, - 2, ...).
    static const int kModalLayerPriority = -130;

    MiModalLayer();
    virtual ~MiModalLayer();

    virtual bool init();  // targeted touches at kModalLayerPriority, close-on-touch on, no-op dismiss callback
    virtual bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event);  // always swallows

    // Runs the dismiss callback, then removes the layer from its parent (with cleanup).
    virtual void dismiss();
    virtual void setDismissCallback(std::function<void()> callback);

    void setEnableCloseOnTouchLayer(bool enable);

protected:
    bool m_closeOnTouch;                       // orig +0x108
    std::function<void()> m_dismissCallback;  // orig +0x10c
};

#endif
