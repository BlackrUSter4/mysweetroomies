#include "ExScenarioData.h"

#include <cstdlib>
#include <cstring>

namespace {

typedef picojson::object JsonObject;

const JsonObject kEmptyObject;

// The original read sub-values straight out of picojson's union without a type check (a JSON typo crashed it).
// These helpers keep its "present -> use it" logic but treat a value of the wrong type as absent.
const JsonObject &objectAt(const JsonObject &json, const std::string &key)
{
    JsonObject::const_iterator it = json.find(key);
    if (it == json.end() || !it->second.is<JsonObject>()) return kEmptyObject;
    return it->second.get<JsonObject>();
}

bool hasObject(const JsonObject &json, const std::string &key)
{
    JsonObject::const_iterator it = json.find(key);
    return it != json.end() && it->second.is<JsonObject>();
}

// orig assignIfExistString 001c2a04 (weak)
void assignIfExistString(std::string &out, const JsonObject &json, const std::string &key)
{
    JsonObject::const_iterator it = json.find(key);
    if (it != json.end() && it->second.is<std::string>()) out = it->second.get<std::string>();
}

// orig assignStringToDouble 001c2a2c: the JSON carries numbers as strings ("x": "120"); plain numbers are accepted too.
void assignStringToDouble(double &out, const JsonObject &json, const std::string &key)
{
    JsonObject::const_iterator it = json.find(key);
    if (it == json.end()) return;
    if (it->second.is<std::string>())
        out = strtod(it->second.get<std::string>().c_str(), NULL);
    else if (it->second.is<double>())
        out = it->second.get<double>();
}

// `table.stringToEnum[json[key]]` in the original; an unknown string maps to value 0 there.
template <typename T>
void assignIfExistEnum(T &out, const JsonObject &json, const std::string &key, const EnumString<T> &table)
{
    JsonObject::const_iterator it = json.find(key);
    if (it != json.end() && it->second.is<std::string>()) out = table.toEnum(it->second.get<std::string>(), T(0));
}

// orig parseJsonVisualParticle 001c2a60
void parseJsonVisualParticle(const JsonObject &json, ExScenarioDataVisualParticle &particle)
{
    assignIfExistString(particle.type, json, "type");
    JsonObject::const_iterator specs = json.find("spec");
    if (specs == json.end() || !specs->second.is<picojson::array>()) return;
    const picojson::array &list = specs->second.get<picojson::array>();
    for (picojson::array::const_iterator it = list.begin(); it != list.end(); ++it) {
        const JsonObject &entry = it->is<JsonObject>() ? it->get<JsonObject>() : kEmptyObject;
        ExVisualParticleSpec spec;
        assignStringToDouble(spec.x, entry, "x");
        assignStringToDouble(spec.y, entry, "y");
        assignIfExistString(spec.path, entry, "path");
        particle.specs.push_back(spec);
    }
}

// orig parseJsonVisualFade 001c30c0
void parseJsonVisualFade(const JsonObject &json, ExScenarioDataVisualFade &fade)
{
    assignIfExistEnum(fade.type, json, "type", ExVisualFadeTypeNames);
    assignIfExistEnum(fade.on, json, "on", ExVisualFadeOnNames);
    assignIfExistEnum(fade.speed, json, "speed", ExVisualFadeSpeedNames);
    assignIfExistString(fade.color, json, "color");
}

// orig parseJsonVisualFilter 001c32fc
void parseJsonVisualFilter(const JsonObject &json, ExScenarioDataVisualFilter &filter)
{
    assignIfExistEnum(filter.type, json, "type", ExVisualFilterTypeNames);
    assignIfExistEnum(filter.on, json, "on", ExVisualFilterOnNames);
    assignIfExistString(filter.option, json, "option");
}

// orig parseJsonVisualManpu 001c34f8
void parseJsonVisualManpu(const JsonObject &json, ExScenarioDataVisualManpu &manpu)
{
    assignIfExistEnum(manpu.location, json, "location", ExVisualManpuLocationNames);
    assignIfExistString(manpu.type, json, "type");
    // Unlike the other keys, a missing transition_time resets the 1000 ms default to 0.
    manpu.transitionTime = 0.0;
    assignStringToDouble(manpu.transitionTime, json, "transition_time");
    JsonObject::const_iterator paths = json.find("path");
    if (paths != json.end() && paths->second.is<picojson::array>()) {
        const picojson::array &list = paths->second.get<picojson::array>();
        for (picojson::array::const_iterator it = list.begin(); it != list.end(); ++it)
            manpu.paths.push_back(it->is<std::string>() ? it->get<std::string>() : std::string());
    }
}

// orig parseJsonSceneFgArray 001c39a0
void parseJsonSceneFgArray(const picojson::value &json, std::vector<ExScenarioDataSceneFg> &out)
{
    const picojson::array &list = json.get<picojson::array>();
    for (picojson::array::const_iterator it = list.begin(); it != list.end(); ++it) {
        const JsonObject &entry = it->is<JsonObject>() ? it->get<JsonObject>() : kEmptyObject;
        ExScenarioDataSceneFg fg;
        assignIfExistString(fg.perspective, entry, "perspective");
        assignIfExistEnum(fg.size, entry, "size", ExScenarioFgSizeNames);
        assignIfExistEnum(fg.position, entry, "position", ExScenarioFgPositionNames);
        assignIfExistString(fg.characterId, entry, "character_id");
        assignIfExistString(fg.appId, entry, "app_id");
        const JsonObject &layers = objectAt(entry, "layer");
        for (JsonObject::const_iterator layer = layers.begin(); layer != layers.end(); ++layer) {
            ExScenarioDataSceneFgLayer images;
            if (layer->second.is<std::string>()) {
                images.images.push_back(layer->second.get<std::string>());
            } else if (layer->second.is<picojson::array>()) {
                const picojson::array &frames = layer->second.get<picojson::array>();
                for (picojson::array::const_iterator f = frames.begin(); f != frames.end(); ++f)
                    images.images.push_back(f->is<std::string>() ? f->get<std::string>() : std::string());
            } else {
                continue;
            }
            fg.layers.insert(std::make_pair(layer->first, images));  // first occurrence wins, as in the original
        }
        out.push_back(fg);
    }
}

}  // namespace

std::shared_ptr<ExScenarioData> ExScenarioData::parseJson(picojson::object &json)
{
    std::shared_ptr<ExScenarioData> data = std::make_shared<ExScenarioData>();

    if (json.find("scene") != json.end()) {
        const JsonObject &scene = objectAt(json, "scene");
        ExScenarioDataScene &out = data->scene;
        assignIfExistString(out.speaker, scene, "speaker");
        out.speakerIsNull = out.speaker == "__NULL__";
        assignIfExistString(out.body, scene, "body");
        assignIfExistString(out.bg, scene, "bg");
        assignIfExistEnum(out.showTail, scene, "show_tail", ExSceneShowTailNames);
        assignIfExistEnum(out.linkScene, scene, "link_scene", ExSceneLinkSceneNames);
        JsonObject::const_iterator fg = scene.find("fg");
        if (fg != scene.end()) {
            if (fg->second.is<picojson::array>()) {
                parseJsonSceneFgArray(fg->second, out.fg);
                out.fgIsString = false;
            } else if (fg->second.is<std::string>()) {
                out.fgIsString = true;
            }
        }
    }

    if (json.find("audio") != json.end()) {
        const JsonObject &audio = objectAt(json, "audio");
        assignIfExistString(data->audio.bgm, audio, "bgm");
        assignIfExistString(data->audio.se, audio, "se");
        assignIfExistString(data->audio.voice, audio, "voice");
    }

    if (json.find("visual") != json.end()) {
        const JsonObject &visual = objectAt(json, "visual");
        ExScenarioDataVisual &out = data->visual;
        if (hasObject(visual, "fade")) parseJsonVisualFade(objectAt(visual, "fade"), out.fade);
        if (hasObject(visual, "camera"))
            assignIfExistEnum(out.cameraType, objectAt(visual, "camera"), "type", ExVisualCameraTypeNames);
        if (hasObject(visual, "filter")) parseJsonVisualFilter(objectAt(visual, "filter"), out.filter);
        if (hasObject(visual, "manpu")) parseJsonVisualManpu(objectAt(visual, "manpu"), out.manpu);
        if (hasObject(visual, "shake"))
            assignIfExistEnum(out.shakeType, objectAt(visual, "shake"), "type", ExVisualShakeTypeNames);
        // The original skips "particle"/"end_mark" only when they are JSON null; anything else that isn't an object
        // would have crashed it.
        if (hasObject(visual, "particle")) parseJsonVisualParticle(objectAt(visual, "particle"), out.particle);
        if (hasObject(visual, "end_mark"))
            assignIfExistEnum(out.endMarkType, objectAt(visual, "end_mark"), "type", ExVisualEndMarkTypeNames);
    }
    return data;
}

std::shared_ptr<ExScenarioData> ExScenarioData::parse(const char *json)
{
    picojson::value root;
    std::string error;
    picojson::parse(root, json, json + strlen(json), &error);
    // The original built a "syntax error at line N near: ..." message here and then ignored it.
    picojson::object empty;
    return parseJson(root.is<picojson::object>() ? root.get<picojson::object>() : empty);
}
