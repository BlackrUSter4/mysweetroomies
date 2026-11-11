#include "ShaderSprite.h"

#include <cstddef>

#include "ResourceManager.h"

USING_NS_CC;

// orig 001de068
ShaderSprite::ShaderSprite() {}

// orig 001ddf9c
ShaderSprite::~ShaderSprite() {}

// orig 001de1e6
bool ShaderSprite::initWithTexture(CCTexture2D *texture, const CCRect &rect)
{
    if (!CCSprite::initWithTexture(texture, rect)) return false;
    initProgram();
    return true;
}

// orig 001de0ac
void ShaderSprite::initProgram()
{
    const std::string path = CCFileUtils::sharedFileUtils()->fullPathForFilename(m_fragmentShaderFile.c_str());
    // The original took c_str() of the slurped temporary after destroying it; keep the source alive until linked.
    const std::string fragmentSource = ResourceManager::getInstance()->slurp(path);

    CCGLProgram *program = new CCGLProgram();
    program->initWithVertexShaderByteArray(ccPositionTextureColor_vert, fragmentSource.c_str());
    setShaderProgram(program);
    program->release();

    getShaderProgram()->addAttribute(kCCAttributeNamePosition, kCCVertexAttrib_Position);
    getShaderProgram()->addAttribute(kCCAttributeNameColor, kCCVertexAttrib_Color);
    getShaderProgram()->addAttribute(kCCAttributeNameTexCoord, kCCVertexAttrib_TexCoords);
    getShaderProgram()->link();
    getShaderProgram()->updateUniforms();
    buildCustomUniforms();
}

// orig 001de1d0
void ShaderSprite::listenBackToForeground(CCObject *sender)
{
    setShaderProgram(nullptr);
    initProgram();
}

// orig 001ddfec
void ShaderSprite::draw()
{
    ccGLEnableVertexAttribs(kCCVertexAttribFlag_PosColorTex);
    const ccBlendFunc blend = getBlendFunc();
    ccGLBlendFunc(blend.src, blend.dst);

    getShaderProgram()->use();
    getShaderProgram()->setUniformsForBuiltins();
    setCustomUniforms();

    ccGLBindTexture2D(getTexture()->getName());

    // Deviation: the original drew without pointing the attributes at this sprite's quad, so it rendered whatever
    // vertex arrays the previous draw call left behind. These are the pointers CCSprite::draw sets up.
    const GLsizei stride = sizeof(m_sQuad.bl);
    const char *quad = reinterpret_cast<const char *>(&m_sQuad);
    glVertexAttribPointer(kCCVertexAttrib_Position, 3, GL_FLOAT, GL_FALSE, stride,
                          quad + offsetof(ccV3F_C4B_T2F, vertices));
    glVertexAttribPointer(kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, stride,
                          quad + offsetof(ccV3F_C4B_T2F, texCoords));
    glVertexAttribPointer(kCCVertexAttrib_Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride,
                          quad + offsetof(ccV3F_C4B_T2F, colors));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CC_INCREMENT_GL_DRAWS(1);
}
