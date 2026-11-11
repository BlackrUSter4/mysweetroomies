// BackgroundRender: the background layer of CanvasLayer (z 0). Children by tag:
//   1  the background: a sprite of ViewBackgroundData::path scaled to the layer width (through FilterData when the
//      scene has a background filter), or a plain colour layer (Reset backgrounds and missing images)
//   2  an optional particle system (ViewBackgroundData::particle), centred horizontally
#ifndef BACKGROUNDRENDER_H
#define BACKGROUNDRENDER_H

#include <memory>

#include "SceneData.h"
#include "cocos2d.h"

class BackgroundRender : public cocos2d::CCLayer {
public:
    static BackgroundRender *create();
    virtual ~BackgroundRender();
    virtual bool init();

    // Replaces the background (tag 1) with what `background` describes.
    void drawImage(std::shared_ptr<ViewBackgroundData> background);

    // Draws the new background if the scene has one (null = keep the current one). `previous` is unused.
    void onPreRender(std::shared_ptr<ViewData> previous, std::shared_ptr<ViewData> current);
    void onMainRender(std::shared_ptr<ViewData> current);  // nothing to do

private:
    cocos2d::CCParticleSystem *m_particle = nullptr;  // running background particles (owned by the node tree)
};

#endif
