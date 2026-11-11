#include "BaseFilter.h"

USING_NS_CC;

// orig 001d6394
BaseFilter::BaseFilter() {}

// orig 001ddacc
BaseFilter::~BaseFilter() {}

// orig 001d63c4
BaseFilter *BaseFilter::create(const std::string &imagePath)
{
    BaseFilter *sprite = new BaseFilter();
    if (!sprite->initWithFile(imagePath.c_str())) {
        delete sprite;
        return nullptr;
    }
    sprite->autorelease();
    return sprite;
}

// orig 001dda24
void BaseFilter::prepare() {}

// orig 001dda28
void BaseFilter::prepare(const std::string &vertexShaderFile, const std::string &fragmentShaderFile)
{
    CCGLProgram *program = new CCGLProgram();
    program->autorelease();
    program->initWithVertexShaderFilename(vertexShaderFile.c_str(), fragmentShaderFile.c_str());
    setShaderProgram(program);

    getShaderProgram()->addAttribute(kCCAttributeNamePosition, kCCVertexAttrib_Position);
    getShaderProgram()->addAttribute(kCCAttributeNameTexCoord, kCCVertexAttrib_TexCoords);
    getShaderProgram()->link();
    getShaderProgram()->updateUniforms();
    setUniformValues();
    getShaderProgram()->use();
}

// orig 001dda26
void BaseFilter::setUniformValues() {}
