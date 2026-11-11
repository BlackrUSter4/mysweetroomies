// BlurSprite: ShaderSprite with "Framework/shader/BlurShader.fsh" (uniforms blurSize, substract). That shader file is
// not shipped in the APK, so the class is dead code in the game; kept because the original exported it.
#ifndef BLURSPRITE_H
#define BLURSPRITE_H

#include "ShaderSprite.h"
#include "cocos2d.h"

class BlurSprite : public ShaderSprite {
public:
    BlurSprite();
    virtual ~BlurSprite();

    // Blur step in texels (1 = one texel).
    void setBlurSize(float texels);

protected:
    virtual void buildCustomUniforms();
    virtual void setCustomUniforms();

private:
    cocos2d::CCPoint m_blurStep;               // texel size times setBlurSize()
    GLfloat m_subtract[4] = {0, 0, 0, 0};      // uniform "substract" (sic), always zero
    GLint m_blurSizeLocation = -1;
    GLint m_subtractLocation = -1;
};

#endif
