// BlurFilter: BaseFilter drawing its image through Framework/shader/GaussianBlur.fsh, a 3x3 gaussian whose step is
// one texel (uniform u_blurSize = 1 / texture size in pixels).
#ifndef BLURFILTER_H
#define BLURFILTER_H

#include <string>

#include "BaseFilter.h"

class BlurFilter : public BaseFilter {
public:
    static const std::string VERTEX_SHADER;    // "Framework/shader/Common.vsh"
    static const std::string FRAGMENT_SHADER;  // "Framework/shader/GaussianBlur.fsh"

    virtual ~BlurFilter();

    // Sprite of `imagePath` / of `frame` (null frame -> null); autoreleased, prepare() not yet called.
    static BaseFilter *create(const std::string &imagePath);
    static BaseFilter *createWithSpriteFrame(cocos2d::CCSpriteFrame *frame);

    virtual void prepare();           // BaseFilter::prepare(VERTEX_SHADER, FRAGMENT_SHADER)
    virtual void setUniformValues();  // u_blurSize
};

#endif
