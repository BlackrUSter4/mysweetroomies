// MosaicFilter: BaseFilter drawing its image through Framework/shader/Mosaic.fsh (30 x 30 blocks per texture).
#ifndef MOSAICFILTER_H
#define MOSAICFILTER_H

#include <string>

#include "BaseFilter.h"

class MosaicFilter : public BaseFilter {
public:
    static const std::string VERTEX_SHADER;    // "Framework/shader/Common.vsh"
    static const std::string FRAGMENT_SHADER;  // "Framework/shader/Mosaic.fsh"

    virtual ~MosaicFilter();

    // Sprite of `imagePath` / of `frame` (null frame -> null); autoreleased, prepare() not yet called.
    static BaseFilter *create(const std::string &imagePath);
    static BaseFilter *createWithSpriteFrame(cocos2d::CCSpriteFrame *frame);

    virtual void prepare();  // BaseFilter::prepare(VERTEX_SHADER, FRAGMENT_SHADER)
};

#endif
