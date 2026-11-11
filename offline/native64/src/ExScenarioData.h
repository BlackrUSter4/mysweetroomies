// ExScenarioData and its parts: one line ("scene") of a story transcript exactly as the JSON gives it, before
// SceneDataParser turns it into render-ready SceneData. Only the parser should need these types.
//
// JSON shape of one entry of ExStoryData's "scenario_data" (see engine_format() in server/app.py):
//   {"scene":  {"speaker", "body", "bg", "fg": [ {...}, ... ] | "<any string>", "show_tail", "link_scene"},
//    "audio":  {"bgm", "se", "voice"},
//    "visual": {"fade": {type, on, speed, color}, "camera": {type}, "filter": {type, on, option},
//               "manpu": {location, type, transition_time, path: [..]}, "shake": {type},
//               "particle": {type, spec: [{x, y, path}, ..]}, "end_mark": {type}}}
// Every key is optional. Missing keys keep the defaults documented below.
//
// Header also holds (all plain data, no functions of their own except implicit copy/destroy):
//   ExScenarioDataScene, ExScenarioDataSceneFg, ExScenarioDataSceneFgLayer, ExScenarioDataAudio,
//   ExScenarioDataVisual + ExScenarioDataVisualFade/Filter/Manpu/Particle, ExVisualParticleSpec.
#ifndef EXSCENARIODATA_H
#define EXSCENARIODATA_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ExEnums.h"
#include "picojson.h"

// One entry of visual.particle.spec: where a particle emitter goes and which .plist it plays.
struct ExVisualParticleSpec {
    double x = 0.0;    // "x" (string or number in JSON), screen units as authored
    double y = 0.0;    // "y"
    std::string path;  // "path": particle .plist; SceneDataParser rewrites it to the resource cache path
};

// The images of one layer of a character sprite: one entry = a still layer, several = animation frames
// (eyes "03_eye", mouth "05_mouth"); JSON value is either a string or an array of strings.
struct ExScenarioDataSceneFgLayer {
    std::vector<std::string> images;  // file names without ".png", relative to the layer directory
};

// One element of scene.fg: a character to show.
struct ExScenarioDataSceneFg {
    std::string perspective;                                   // "perspective": pose/angle directory name
    std::string characterId;                                   // "character_id"
    ExScenarioFgSize size = ExScenarioFgSize_M;                // "size": s|m|l
    ExScenarioFgPosition position = ExScenarioFgPosition_Middle;  // "position": l|m|r
    std::string appId;                                         // "app_id": character set (top dir of the offsets)
    std::map<std::string, ExScenarioDataSceneFgLayer> layers;  // "layer": {layer name: image | [images]}
};

// "scene": text and picture of the line.
struct ExScenarioDataScene {
    std::string bg;                          // "bg": background image name ("__NULL__" = reset to black)
    std::vector<ExScenarioDataSceneFg> fg;   // "fg" when it is an array
    bool fgIsString = false;                 // "fg" was a string (any): clear every character on screen
    std::string body;                        // "body": the text
    std::string speaker;                     // "speaker": name shown in the name plate
    bool speakerIsNull = false;              // speaker == "__NULL__": narration, no name plate
    ExSceneShowTail showTail = ExSceneShowTail_None;  // "show_tail"
    ExSceneLinkScene linkScene = ExSceneLinkScene_None;  // "link_scene"
};

// "audio": file names as in the JSON (SceneDataParser resolves them). "__NULL__" means stop.
struct ExScenarioDataAudio {
    std::string bgm;    // "bgm": looping music; empty = keep what is playing
    std::string se;     // "se": one-shot sound effect
    std::string voice;  // "voice": voice line (also drives the lip-sync length, see SceneDataParser)
};

struct ExScenarioDataVisualFade {
    ExVisualFadeOn on = ExVisualFadeOn_None;           // "on"; None = no fade this scene
    ExVisualFadeType type = ExVisualFadeType_None;     // "type"
    ExVisualFadeSpeed speed = ExVisualFadeSpeed_None;  // "speed"
    std::string color;                                 // "color": ColorString syntax (e.g. "#000000")
};

struct ExScenarioDataVisualFilter {
    ExVisualFilterOn on = ExVisualFilterOn_None;        // "on"
    ExVisualFilterType type = ExVisualFilterType_None;  // "type"
    std::string option = "0";                           // "option": integer as string (blur radius)
};

struct ExScenarioDataVisualManpu {
    std::string type;                 // "type": "none" = remove the manpu; anything else (incl. "") shows `paths`
    std::vector<std::string> paths;   // "path": image paths, resolved with ResourceManager::getCachePath
    // "transition_time" in milliseconds. 1000 when the "manpu" object is absent, but 0 when the object is present
    // without this key (both as in the original).
    double transitionTime = 1000.0;
    ExVisualManpuLocation location = ExVisualManpuLocation_Right;  // "location"
};

struct ExScenarioDataVisualParticle {
    std::string type = "none";               // "type": "none" = nothing, "__NULL__" = stop particles, else play
    std::vector<ExVisualParticleSpec> specs;  // "spec"
};

// "visual": screen effects of the line.
struct ExScenarioDataVisual {
    ExScenarioDataVisual() {}
    ExScenarioDataVisual(const ExScenarioDataVisual &) = default;
    ~ExScenarioDataVisual() {}

    ExScenarioDataVisualFade fade;                       // "fade"
    ExVisualCameraType cameraType = ExVisualCameraType_None;  // "camera": {"type"}
    ExScenarioDataVisualFilter filter;                   // "filter"
    ExScenarioDataVisualManpu manpu;                     // "manpu"
    ExVisualShakeType shakeType = ExVisualShakeType_None;  // "shake": {"type"}
    ExScenarioDataVisualParticle particle;               // "particle"
    ExVisualEndMarkType endMarkType = ExVisualEndMarkType_None;  // "end_mark": {"type"}
};

class ExScenarioData {
public:
    ExScenarioDataScene scene;
    ExScenarioDataVisual visual;
    ExScenarioDataAudio audio;

    // Parses one line from its JSON text. Invalid JSON yields a line with all defaults. orig 001d455c
    static std::shared_ptr<ExScenarioData> parse(const char *json);
    // Builds one line from an already parsed JSON object (an element of "scenario_data"). orig 001d3e5c
    static std::shared_ptr<ExScenarioData> parseJson(picojson::object &json);
};

#endif
