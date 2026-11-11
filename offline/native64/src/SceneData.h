// SceneData: one story line ready to render, produced by SceneDataParser from an ExScenarioData and handed to every
// SceneEventListener by SceneManager (see SceneManager.h for when). Everything is plain shared data; listeners must
// treat it as read-only because the same objects are reused for the text log, the previous-scene argument, etc.
//
// Conventions shared by every sub-struct:
//   * a null shared_ptr / empty string means "no change from the previous scene",
//   * "__RESET__" in a path means "remove/stop it" (the JSON said "__NULL__"),
//   * other paths are absolute paths into the resource cache (ResourceManager::getCachePath) and include ".png"
//     for images.
//
// Header also holds SoundData, ViewData, ViewBackgroundData, ViewPersonData, ViewEffectData, ManpuData, FadeData
// (structs with no functions of their own). TextData has its own header.
#ifndef SCENEDATA_H
#define SCENEDATA_H

#include <memory>
#include <string>
#include <vector>

#include "ExEnums.h"
#include "ExScenarioData.h"  // ExVisualParticleSpec
#include "FilterData.h"      // render subsystem: { ExVisualFilterType type; int option; ... }
#include "LayerData.h"
#include "TextData.h"
#include "cocos2d.h"

// Sound changes of the scene (SoundEngine reads it from SceneData::sound).
struct SoundData {
    std::string bgm;    // ""= keep current BGM, "__RESET__" = stop BGM, else path of the track to loop
    std::string se;     // ""= nothing, "__RESET__" = stop SE, else path of the effect to play once
    std::string voice;  // ""= nothing, "__RESET__" = stop voice, else path of the voice line
};

// ViewBackgroundData::type
enum ViewBackgroundType {
    ViewBackgroundType_None = 0,   // only ever seen before the parser fills it in
    ViewBackgroundType_Reset = 1,  // "__NULL__" in the JSON: plain `color` layer, path is "__RESET__"
    ViewBackgroundType_Image = 2,  // `path` is the background picture
};

struct ViewBackgroundData {
    int type = ViewBackgroundType_None;  // ViewBackgroundType
    std::string path;                    // image path (Image) or "__RESET__" (Reset)
    cocos2d::ccColor4B color;            // colour of the Reset layer: opaque black (only set for Reset)
    // Optional particle .plist to run on top of the background (BackgroundRender::onPreRender). The scene parser
    // never sets it.
    std::shared_ptr<std::string> particle;
    FilterData filter;  // visual.filter when it applies to "bg"/"both", else type None
};

// ViewPersonData::position
enum ViewPersonPosition {
    ViewPersonPosition_Center = 0,  // fg position "m"
    ViewPersonPosition_Left = 1,    // fg position "l"
    ViewPersonPosition_Right = 2,   // fg position "r"
};

// One character on screen.
struct ViewPersonData {
    int position = ViewPersonPosition_Center;  // ViewPersonPosition; CanvasLayer has one PersonRender per slot
    // GUESS: a string the original allocates but neither the parser nor the renderers fill or read.
    std::string unusedName;
    // Layers back to front (offset file "order"): OffsetLayerData for stills, FrameAnimationData for animated ones.
    std::vector<std::shared_ptr<LayerData>> layers;
    FilterData filter;  // visual.filter when it applies to "fg"/"both", else type None
};

// visual.manpu, converted.
struct ManpuData {
    double transitionTime = 0.0;      // seconds (JSON milliseconds / 1000)
    std::vector<std::string> paths;   // images to cycle through
    ExVisualManpuLocation location = ExVisualManpuLocation_Right;
};

// visual.fade, converted.
struct FadeData {
    cocos2d::ccColor4B color = {0, 0, 0, 255};  // ColorString::toColor(visual.fade.color)
    float duration = 1.0f;                      // seconds, from visual.fade.speed (0 / 0.25 / 1 / 3)
    ExVisualFadeType type = ExVisualFadeType_None;
    int reserved = 2;  // GUESS: initialised to 2 by the original, never read by any renderer
    ExVisualFadeOn on = ExVisualFadeOn_None;  // never None in practice: an "on": "none" fade becomes a null FadeData
};

// Screen effects of the scene (EffectRender, CanvasLayer).
struct ViewEffectData {
    std::shared_ptr<ManpuData> manpu;  // null when visual.manpu.type is "none"
    ExVisualCameraType camera = ExVisualCameraType_None;
    std::shared_ptr<FadeData> fade;    // null = no fade
    std::vector<ExVisualParticleSpec> particles;  // emitters to start; paths already resolved to the cache
    bool stopParticles = false;        // visual.particle.type was "__NULL__"
    ExVisualShakeType shake = ExVisualShakeType_None;
    ExVisualEndMarkType endMark = ExVisualEndMarkType_None;
};

// Everything drawn for the scene.
struct ViewData {
    std::shared_ptr<ViewBackgroundData> background;  // null = keep the previous background (scene.bg was "")
    bool reserved8 = false;  // GUESS: zero-initialised and never used by the original
    // Characters of this scene. Empty with resetPersons == false means "keep whoever is on screen".
    std::vector<std::shared_ptr<ViewPersonData>> persons;
    bool resetPersons = false;  // scene.fg was a string: remove every character
    std::shared_ptr<ViewEffectData> effect;  // always set by the parser
    bool reserved24 = false;  // GUESS: zero-initialised and never used by the original
};

struct SceneData {
    std::shared_ptr<TextData> text;    // always set by the parser
    std::shared_ptr<SoundData> sound;  // always set by the parser
    std::shared_ptr<ViewData> view;    // always set by the parser (view->effect too)
};

#endif
