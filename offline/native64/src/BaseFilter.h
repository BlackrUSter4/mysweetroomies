// BaseFilter: a CCSprite that can carry one of the game's filter shaders (Framework/shader/*.fsh). Subclasses
// (NoFilter, SepiaFilter, GrayScaleFilter, InvertFilter, BlurFilter, MosaicFilter) only differ in the shader file
// their prepare() loads; FilterData::createFilter picks one and calls prepare() on it.
//
//   BaseFilter *sprite = SepiaFilter::create(path);  // CCSprite::initWithFile, autoreleased
//   sprite->prepare();                                // compile + link Common.vsh / Sepia.fsh, set uniforms
#ifndef BASEFILTER_H
#define BASEFILTER_H

#include <string>

#include "cocos2d.h"

class BaseFilter : public cocos2d::CCSprite {
public:
    BaseFilter();
    virtual ~BaseFilter();

    // Plain sprite of `imagePath` without any shader of its own (FilterData type None). Null on failure.
    static BaseFilter *create(const std::string &imagePath);

    // Installs the filter's GL program. The base version does nothing.
    virtual void prepare();
    // Builds a program from the two shader files (paths as CCFileUtils resolves them), binds a_position /
    // a_texCoord, links, calls setUniformValues() and leaves the program in use.
    virtual void prepare(const std::string &vertexShaderFile, const std::string &fragmentShaderFile);
    // Hook for filters with extra uniforms (BlurFilter). The base version does nothing.
    virtual void setUniformValues();
};

#endif
