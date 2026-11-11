#include "BlurFilter.h"

USING_NS_CC;

const std::string BlurFilter::VERTEX_SHADER = "Framework/shader/Common.vsh";
const std::string BlurFilter::FRAGMENT_SHADER = "Framework/shader/GaussianBlur.fsh";

// orig 001ddb48
BlurFilter::~BlurFilter() {}

// orig 001d66d8
BaseFilter *BlurFilter::create(const std::string &imagePath)
{
    BlurFilter *sprite = new BlurFilter();
    if (!sprite->initWithFile(imagePath.c_str())) {
        delete sprite;
        return nullptr;
    }
    sprite->autorelease();
    return sprite;
}

// orig 001d674c
BaseFilter *BlurFilter::createWithSpriteFrame(CCSpriteFrame *frame)
{
    BlurFilter *sprite = new BlurFilter();
    if (frame == nullptr || !sprite->initWithSpriteFrame(frame)) {
        delete sprite;
        return nullptr;
    }
    sprite->autorelease();
    return sprite;
}

// orig 001ddb5c
void BlurFilter::prepare()
{
    BaseFilter::prepare(VERTEX_SHADER, FRAGMENT_SHADER);
}

// orig 001ddb78
void BlurFilter::setUniformValues()
{
    // One texel in texture coordinates: the 3x3 kernel samples the immediate neighbours.
    const CCSize pixels = getTexture()->getContentSizeInPixels();
    const GLint location = glGetUniformLocation(getShaderProgram()->getProgram(), "u_blurSize");
    glUniform2f(location, 1.0f / pixels.width, 1.0f / pixels.height);
}
