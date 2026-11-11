// GrayScaleFilter: BaseFilter drawing its image through Framework/shader/GrayScale.fsh (gray scale).
#ifndef GRAYSCALEFILTER_H
#define GRAYSCALEFILTER_H

#include <string>

#include "BaseFilter.h"

class GrayScaleFilter : public BaseFilter {
public:
    static const std::string VERTEX_SHADER;    // "Framework/shader/Common.vsh"
    static const std::string FRAGMENT_SHADER;  // "Framework/shader/GrayScale.fsh"

    virtual ~GrayScaleFilter();

    // Sprite of `imagePath` / of `frame` (null frame -> null); autoreleased, prepare() not yet called.
    static BaseFilter *create(const std::string &imagePath);
    static BaseFilter *createWithSpriteFrame(cocos2d::CCSpriteFrame *frame);

    virtual void prepare();  // BaseFilter::prepare(VERTEX_SHADER, FRAGMENT_SHADER)
};

#endif
