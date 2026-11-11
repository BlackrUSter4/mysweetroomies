#include "RGBAColorLayer.h"

USING_NS_CC;

// orig 001e1ba0
RGBAColorLayer *RGBAColorLayer::create()
{
    RGBAColorLayer *layer = new RGBAColorLayer();
    if (!layer->init()) {
        delete layer;
        return nullptr;
    }
    layer->autorelease();
    layer->m_opacity = 255;
    return layer;
}

// orig 001e1aac
RGBAColorLayer::~RGBAColorLayer() {}

// orig 001e1a90
GLubyte RGBAColorLayer::getOpacity()
{
    return m_opacity;
}

// orig 001e1b2c
void RGBAColorLayer::setOpacity(GLubyte opacity)
{
    m_opacity = opacity;
    CCArray *children = getChildren();
    if (children == nullptr) return;
    CCObject *child = nullptr;
    CCARRAY_FOREACH(children, child)
    {
        CCRGBAProtocol *rgba = dynamic_cast<CCRGBAProtocol *>(child);
        if (rgba != nullptr) rgba->setOpacity(opacity);
    }
}
