// ShaderSprite: a CCSprite drawn with its own fragment shader (the cocos2d-x 2.x ShaderTest "ShaderSprite" pattern).
// Subclasses set the fragment shader file in their constructor and provide the uniforms; the vertex shader is the
// engine's ccPositionTextureColor_vert. Only BlurSprite derives from it, and nothing in the game creates one.
#ifndef SHADERSPRITE_H
#define SHADERSPRITE_H

#include <string>

#include "cocos2d.h"

class ShaderSprite : public cocos2d::CCSprite {
public:
    ShaderSprite();
    virtual ~ShaderSprite();

    // CCSprite::initWithTexture, then initProgram().
    virtual bool initWithTexture(cocos2d::CCTexture2D *texture, const cocos2d::CCRect &rect);
    virtual void draw();

    // Reads m_fragmentShaderFile, builds/links the program, then buildCustomUniforms().
    void initProgram();
    // GL context came back: drop the dead program and rebuild it.
    void listenBackToForeground(cocos2d::CCObject *sender);

protected:
    virtual void buildCustomUniforms() = 0;  // look up uniform locations after linking
    virtual void setCustomUniforms() = 0;    // upload them before each draw

    std::string m_fragmentShaderFile;
};

#endif
