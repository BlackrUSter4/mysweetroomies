// EffectRender: the effect layer of CanvasLayer (z 8): particles, manpu (comic emotion marks) and the end-of-part
// marks. It is its own SceneManager listener ("effect_render"); MainRender locks the scene (owner "effect_render")
// until the end mark has played, or unlocks right away when there is none.
//
// Children by tag: 0 manpu sprite, 1 particles, 2 end mark ("continued.png" / "fin.png" from
// Framework/ui/conversation.plist).
#ifndef EFFECTRENDER_H
#define EFFECTRENDER_H

#include <memory>

#include "ExEnums.h"
#include "SceneData.h"
#include "cocos2d.h"

class EffectRender : public cocos2d::CCLayer {
public:
    static EffectRender *create();
    virtual ~EffectRender();
    virtual bool init();

    void onPreRender(std::shared_ptr<ViewData> previous, std::shared_ptr<ViewData> current);  // drops manpu + mark
    void onPreEffect(std::shared_ptr<ViewData> previous, std::shared_ptr<ViewData> current);  // nothing
    void onMainRender(std::shared_ptr<ViewData> current);

    void onRenderParticle(std::shared_ptr<ViewEffectData> effect);
    void onRenderManpu(std::shared_ptr<ViewEffectData> effect);
    void onRenderEndMark(ExVisualEndMarkType type);
    void renderEndMarkContinue();
    void renderEndMarkEnd();
    void onFinishEndMark();  // unlocks the scene
};

#endif
