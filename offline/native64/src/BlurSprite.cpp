#include "BlurSprite.h"

USING_NS_CC;

// orig 001ddd04
BlurSprite::BlurSprite()
{
    m_fragmentShaderFile = "Framework/shader/BlurShader.fsh";
}

// orig 001ddbd8
BlurSprite::~BlurSprite() {}

// orig 001ddc20
void BlurSprite::buildCustomUniforms()
{
    const CCSize pixels = getTexture()->getContentSizeInPixels();
    m_blurStep = CCPoint(1.0f / pixels.width, 1.0f / pixels.height);
    for (GLfloat &component : m_subtract) component = 0.0f;

    m_subtractLocation = glGetUniformLocation(getShaderProgram()->getProgram(), "substract");
    m_blurSizeLocation = glGetUniformLocation(getShaderProgram()->getProgram(), "blurSize");
}

// orig 001ddcc4
void BlurSprite::setCustomUniforms()
{
    getShaderProgram()->setUniformLocationWith2f(m_blurSizeLocation, m_blurStep.x, m_blurStep.y);
    getShaderProgram()->setUniformLocationWith4fv(m_subtractLocation, m_subtract, 1);
}

// orig 001ddd5c
void BlurSprite::setBlurSize(float texels)
{
    const CCSize pixels = getTexture()->getContentSizeInPixels();
    m_blurStep = CCPoint(1.0f / pixels.width, 1.0f / pixels.height) * texels;
}
