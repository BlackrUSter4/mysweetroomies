// RGBAColorLayer: a CCLayerRGBA whose opacity is passed straight down to every direct child that supports opacity
// (CCRGBAProtocol), so a whole group of sprites can be faded with one CCFadeTo/CCFadeInTo on the layer. Used by
// OverlayMenuLayer.
#ifndef RGBACOLORLAYER_H
#define RGBACOLORLAYER_H

#include "cocos2d.h"

class RGBAColorLayer : public cocos2d::CCLayerRGBA {
public:
    static RGBAColorLayer *create();  // opacity starts at 255
    virtual ~RGBAColorLayer();

    virtual GLubyte getOpacity();
    // Stores the value and sets it on each direct child; the layer's own CCLayerRGBA opacity is left alone.
    virtual void setOpacity(GLubyte opacity);

private:
    GLubyte m_opacity = 0;
};

#endif
