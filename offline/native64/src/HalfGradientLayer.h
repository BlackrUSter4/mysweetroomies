// HalfGradientLayer: the "curtain" CanvasLayer slides in for from_center / to_center fades. A square of the screen
// width, its top half a solid colour (child tag 0), its bottom half a gradient from that colour to transparent
// (child tag 1), so the moving edge looks soft.
#ifndef HALFGRADIENTLAYER_H
#define HALFGRADIENTLAYER_H

#include "cocos2d.h"

class HalfGradientLayer : public cocos2d::CCLayer {
public:
    static HalfGradientLayer *create();
    virtual ~HalfGradientLayer();
    virtual bool init();

    // Solid half: color + color.a; gradient half: color at color.a fading to fully transparent.
    void setLayerColor(cocos2d::ccColor4B color);
};

#endif
