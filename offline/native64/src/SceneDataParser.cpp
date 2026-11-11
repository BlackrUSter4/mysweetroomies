#include "SceneDataParser.h"

#include <cmath>
#include <cstring>
#include <map>

#include "ColorString.h"
#include "FrameAnimationData.h"
#include "MediaConnector.h"
#include "OffsetLayerData.h"
#include "ResourceManager.h"

USING_NS_CC;

namespace {

// --- Constants the original kept in a header shared by several translation units ---------------------------------

const char *const kNullMarker = "__NULL__";    // JSON: "remove / stop this"
const char *const kResetMarker = "__RESET__";  // SceneData: "remove / stop this"
const char *const kCharacterDir = "img/resource/character/";
const char *const kBackgroundDir = "img/resource/bg/";
const char *const kImageExtension = ".png";
// The original wrapped audio names in a prefix and suffix that are both empty strings, so audio JSON values are
// already paths relative to the cache ("sound/bgm/....mp3").
const char *const kSoundPrefix = "";
const char *const kSoundSuffix = "";

const char *const kEyeLayer = "03_eye";
const char *const kMouthLayer = "05_mouth";

// Blink: 0 = open, 1 = half, 2 = closed. 212 steps played over kAnimationDurations["03_eye"] seconds (two blinks).
const int kEyeSequence[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
// Talking without a voice file: 0 = closed, 2 = open, 130 steps over 3.25 s.
const int kMouthSequence[] = {
    0, 0, 0, 0, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 2, 2,
    2, 2, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 2};
// Lip sync with a voice file: one second of mouth movement, repeated once per whole second of audio.
const int kLipSyncStart[] = {2};
const int kLipSyncSecond[] = {2, 2, 2, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 2,
                              2, 2, 2, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 2};

template <size_t N>
std::vector<int> toVector(const int (&values)[N])
{
    return std::vector<int>(values, values + N);
}

const std::map<std::string, float> &animationDurations()
{
    static const std::map<std::string, float> durations = {{kMouthLayer, 3.25f}, {kEyeLayer, 6.0f}};
    return durations;
}

const std::map<std::string, std::vector<int>> &animationSequences()
{
    static const std::map<std::string, std::vector<int>> sequences = {{kMouthLayer, toVector(kMouthSequence)},
                                                                      {kEyeLayer, toVector(kEyeSequence)}};
    return sequences;
}

// Advances on every blinking layer parsed so the characters on screen don't blink in unison (the original's
// file-static counter, starting at 1).
int blinkCounter = 1;

// The original appended each second of lip sync with insert() followed by std::inplace_merge over the old and new
// parts. The input isn't sorted, so the result depends on the algorithm; this reproduces GCC 4.9 libstdc++'s
// buffered merge (__move_merge_adaptive / __move_merge_adaptive_backward) so the mouth moves exactly as it did.
void legacyInplaceMerge(std::vector<int> &values, size_t middle)
{
    const size_t length1 = middle, length2 = values.size() - middle;
    if (length1 == 0 || length2 == 0) return;
    if (length1 <= length2) {
        std::vector<int> buffer(values.begin(), values.begin() + middle);
        size_t first1 = 0, first2 = middle, out = 0;
        while (first1 != buffer.size() && first2 != values.size()) {
            if (values[first2] < buffer[first1])
                values[out++] = values[first2++];
            else
                values[out++] = buffer[first1++];
        }
        while (first1 != buffer.size()) values[out++] = buffer[first1++];
    } else {
        std::vector<int> buffer(values.begin() + middle, values.end());
        size_t result = values.size();
        size_t last1 = middle - 1, last2 = buffer.size() - 1;
        while (true) {
            if (buffer[last2] < values[last1]) {
                values[--result] = values[last1];
                if (last1 == 0) {
                    for (size_t i = last2 + 1; i-- > 0;) values[--result] = buffer[i];
                    return;
                }
                --last1;
            } else {
                values[--result] = buffer[last2];
                if (last2 == 0) return;
                --last2;
            }
        }
    }
}

std::string cachePath(const std::string &relativePath)
{
    return ResourceManager::getInstance()->getCachePath(relativePath);
}

// orig parseFilter 001c72e0 (weak). Only sepia/gray/invert/blur are honoured; mosaic and disabled filters clear it.
void parseFilter(FilterData &filter, const ExScenarioDataVisualFilter &source, bool enabled)
{
    if (enabled) {
        switch (source.type) {
        case ExVisualFilterType_Sepia:
        case ExVisualFilterType_GrayScale:
        case ExVisualFilterType_Invert:
            filter.type = source.type;
            return;
        case ExVisualFilterType_Blur:
            filter.type = ExVisualFilterType_Blur;
            filter.setOption(source.option);
            return;
        default:
            break;
        }
    }
    filter.type = ExVisualFilterType_None;
}

// orig generatePersonFilterImage 001c73d0 (weak): pre-render every image of the person through the filter.
void generatePersonFilterImage(std::shared_ptr<ViewPersonData> person, FilterData &filter)
{
    for (size_t i = 0; i < person->layers.size(); ++i) {
        std::shared_ptr<LayerData> layer = person->layers[i];
        std::shared_ptr<FrameAnimationData> animation = std::dynamic_pointer_cast<FrameAnimationData>(layer);
        if (!animation) {
            if (layer->path != "") filter.generateFilteredImage(layer->path);
            continue;
        }
        for (size_t f = 0; f < animation->frames.size(); ++f) {
            if (animation->frames[f]->path != "") filter.generateFilteredImage(animation->frames[f]->path);
        }
    }
}

// orig parseText 001d9788 (weak)
void parseText(std::shared_ptr<SceneData> scene, std::shared_ptr<ExScenarioData> scenario)
{
    ExScenarioDataScene source = scenario->scene;
    std::shared_ptr<TextData> text = std::make_shared<TextData>();
    scene->text = text;

    if (source.speakerIsNull) {
        text->speakerHidden = true;
        text->speaker = "";
    } else {
        text->speakerHidden = false;
        if (source.speaker.empty())
            text->speaker.clear();
        else
            text->speaker = source.speaker == kNullMarker ? std::string() : source.speaker;
    }

    if (source.body.empty())
        text->body.clear();
    else
        text->body = source.body == kNullMarker ? std::string() : source.body;

    text->linkScene = source.linkScene;
    text->showTail = source.showTail;
}

// "" stays "", "__NULL__" becomes "__RESET__", anything else becomes a cache path. Shared by bgm/se/voice.
void resolveSound(std::string &out, const std::string &name)
{
    if (name.empty())
        out.clear();
    else if (name == kNullMarker)
        out = kResetMarker;
    else
        out = cachePath(std::string(kSoundPrefix) + name + kSoundSuffix);
}

// orig parseSound 001c80b4 (weak)
void parseSound(std::shared_ptr<SceneData> scene, std::shared_ptr<ExScenarioData> scenario)
{
    ExScenarioDataAudio audio = scenario->audio;
    scene->sound = std::make_shared<SoundData>();
    resolveSound(scene->sound->bgm, audio.bgm);
    resolveSound(scene->sound->se, audio.se);
    resolveSound(scene->sound->voice, audio.voice);
}

// orig parseBackground 001d98e0 (weak)
void parseBackground(std::shared_ptr<SceneData> scene, std::shared_ptr<ExScenarioData> scenario)
{
    ExScenarioDataScene source = scenario->scene;
    if (source.bg.empty()) return;

    std::shared_ptr<ViewBackgroundData> background = std::make_shared<ViewBackgroundData>();
    scene->view->background = background;
    ExScenarioDataVisual visual = scenario->visual;

    if (source.bg == kNullMarker) {
        background->type = ViewBackgroundType_Reset;
        background->color = ccc4(0, 0, 0, 255);
        background->path = kResetMarker;
    } else {
        background->type = ViewBackgroundType_Image;
        background->path = cachePath(std::string(kBackgroundDir) + source.bg + kImageExtension);
    }
    parseFilter(background->filter, visual.filter,
                visual.filter.on == ExVisualFilterOn_Both || visual.filter.on == ExVisualFilterOn_Bg);
    if (!background->path.empty() && background->path != kResetMarker)
        background->filter.generateFilteredImage(background->path);
}

// Fills `animation` (duration + sequence) for an animated layer called `layerName`. Part of orig parsePerson.
void buildLayerAnimation(FrameAnimationData &animation, const std::string &layerName, const std::string &voicePath)
{
    const char *voice = voicePath.c_str();
    const char *layer = layerName.c_str();

    if ((strstr(voice, ".mp3") || strstr(voice, ".ogg")) && strstr(layer, kMouthLayer)) {
        // Lip sync: one second of movement per whole second of the voice line.
        float seconds = MediaConnector::getAudioDurationTimeSecond(voice);
        double wholeSeconds = floor((double)seconds);
        std::vector<int> sequence = toVector(kLipSyncStart);
        for (int i = 0; (double)i < wholeSeconds; ++i) {
            size_t oldSize = sequence.size();
            sequence.insert(sequence.end(), kLipSyncSecond, kLipSyncSecond + sizeof(kLipSyncSecond) / sizeof(int));
            legacyInplaceMerge(sequence, oldSize);
        }
        animation.duration = (float)wholeSeconds;
        animation.sequence = sequence;
        return;
    }

    std::map<std::string, std::vector<int>>::const_iterator sequence = animationSequences().find(layerName);
    if (strstr(layer, kEyeLayer)) {
        // Stagger blinks: 0, 1 or 2 extra stretches of 7 open-eye frames, and a 5/6/7 s cycle.
        std::vector<int> frames;
        if (sequence != animationSequences().end()) frames = sequence->second;  // GUESS: original threw if absent
        for (int i = 0; i < blinkCounter % 3; ++i) frames.insert(frames.begin(), 7, 0);
        ++blinkCounter;
        animation.duration = (float)(blinkCounter % 3) + 5.0f;
        animation.sequence = frames;
        return;
    }

    // Other animated layers use the fixed tables; the original threw std::out_of_range for names not in them.
    std::map<std::string, float>::const_iterator duration = animationDurations().find(layerName);
    if (duration != animationDurations().end()) animation.duration = duration->second;
    if (sequence != animationSequences().end()) animation.sequence = sequence->second;
    if (duration == animationDurations().end() || sequence == animationSequences().end())
        CCLOG("SceneDataParser: no animation table for layer %s", layer);
}

// Looks up a layer offset; the original used map::at (throwing) for x and map::operator[] (inserting) for y.
CCPoint layerOffset(ExOffsetData &offsets, const ExOffsetDataPerspectiveKey &key)
{
    std::map<ExOffsetDataPerspectiveKey, CCPoint>::const_iterator it = offsets.offset.find(key);
    if (it != offsets.offset.end()) return it->second;
    CCLOG("SceneDataParser: no offset for %s/%s/%s/%s/%s", key.appId.c_str(), key.characterId.c_str(),
          key.perspective.c_str(), key.size.c_str(), key.layer.c_str());
    return CCPointZero;
}

// orig parsePerson 001c8c04 (weak)
void parsePerson(std::shared_ptr<ViewPersonData> person, const ExScenarioDataSceneFg &fg,
                 std::shared_ptr<ExOffsetData> offsets, std::shared_ptr<SceneData> scene)
{
    if (fg.position == ExScenarioFgPosition_Left)
        person->position = ViewPersonPosition_Left;
    else if (fg.position == ExScenarioFgPosition_Right)
        person->position = ViewPersonPosition_Right;
    else
        person->position = ViewPersonPosition_Center;

    ExOffsetDataOrderKey orderKey(fg.appId, fg.characterId, fg.perspective);
    std::map<ExOffsetDataOrderKey, std::vector<std::string>>::const_iterator order = offsets->order.find(orderKey);
    if (order == offsets->order.end()) {
        // The original's map::at threw here, ending the app; a missing character is now just not drawn.
        CCLOG("SceneDataParser: no layer order for %s/%s/%s", fg.appId.c_str(), fg.characterId.c_str(),
              fg.perspective.c_str());
        return;
    }

    const std::string sizeName = ExScenarioFgSizeNames.toString(fg.size);
    const std::vector<std::string> &layerNames = order->second;
    for (size_t i = 0; i < layerNames.size(); ++i) {
        const std::string &layerName = layerNames[i];
        ExOffsetDataPerspectiveKey offsetKey(fg.appId, fg.characterId, fg.perspective, sizeName, layerName);
        const std::string layerDir = std::string(kCharacterDir) + fg.appId + "/" + fg.characterId + "/" +
                                     fg.perspective + "/" + sizeName + "/layer/" + layerName + "/";

        std::map<std::string, ExScenarioDataSceneFgLayer>::const_iterator images = fg.layers.find(layerName);
        if (images == fg.layers.end() || images->second.images.empty()) continue;

        if (images->second.images.size() == 1) {
            std::shared_ptr<OffsetLayerData> still = std::make_shared<OffsetLayerData>();
            still->name = layerName;
            still->path = cachePath(layerDir + images->second.images[0] + kImageExtension);
            CCPoint point = layerOffset(*offsets, offsetKey);
            still->x = point.x;
            still->y = point.y;
            person->layers.push_back(still);
            continue;
        }

        std::shared_ptr<FrameAnimationData> animation = std::make_shared<FrameAnimationData>();
        buildLayerAnimation(*animation, layerName, scene->sound->voice);
        animation->name = layerName;
        CCPoint point = layerOffset(*offsets, offsetKey);
        for (size_t f = 0; f < images->second.images.size(); ++f) {
            std::shared_ptr<OffsetLayerData> frame = std::make_shared<OffsetLayerData>();
            frame->x = point.x;
            frame->y = point.y;
            frame->path = cachePath(layerDir + images->second.images[f] + kImageExtension);
            animation->frames.push_back(frame);
        }
        person->layers.push_back(animation);
    }
}

// orig parsePersonArray 001c9ac4 (weak)
void parsePersonArray(std::shared_ptr<SceneData> scene, std::shared_ptr<ExScenarioData> scenario,
                      std::shared_ptr<ExOffsetData> offsets)
{
    ExScenarioDataScene source = scenario->scene;
    ViewData &view = *scene->view;
    view.persons.clear();

    if (source.fgIsString) {
        view.resetPersons = true;
        return;
    }
    if (source.fg.empty()) {
        view.resetPersons = false;
        return;
    }

    ExScenarioDataVisual visual = scenario->visual;
    const bool filterPersons = visual.filter.on == ExVisualFilterOn_Fg || visual.filter.on == ExVisualFilterOn_Both;
    for (size_t i = 0; i < source.fg.size(); ++i) {
        std::shared_ptr<ViewPersonData> person = std::make_shared<ViewPersonData>();
        parsePerson(person, source.fg[i], offsets, scene);
        parseFilter(person->filter, visual.filter, filterPersons);
        generatePersonFilterImage(person, person->filter);
        view.persons.push_back(person);
    }
}

// orig parseManpu 001c8500 (weak)
void parseManpu(std::shared_ptr<SceneData> scene, std::shared_ptr<ExScenarioData> scenario)
{
    ExScenarioDataVisual visual = scenario->visual;
    std::shared_ptr<ViewEffectData> effect = scene->view->effect;
    if (visual.manpu.type == "none") {
        effect->manpu.reset();
        return;
    }
    std::shared_ptr<ManpuData> manpu = std::make_shared<ManpuData>();
    effect->manpu = manpu;
    manpu->location = visual.manpu.location;
    manpu->transitionTime = visual.manpu.transitionTime / 1000.0;
    for (size_t i = 0; i < visual.manpu.paths.size(); ++i) manpu->paths.push_back(cachePath(visual.manpu.paths[i]));
}

// orig parseFade 001c7f9c (weak)
void parseFade(std::shared_ptr<FadeData> &out, const ExScenarioDataVisualFade &fade)
{
    if (fade.on == ExVisualFadeOn_None) {
        out.reset();
        return;
    }
    out = std::make_shared<FadeData>();
    switch (fade.speed) {
    case ExVisualFadeSpeed_Fast: out->duration = 0.25f; break;
    case ExVisualFadeSpeed_Medium: out->duration = 1.0f; break;
    case ExVisualFadeSpeed_Slow: out->duration = 3.0f; break;
    default: out->duration = 0.0f; break;
    }
    out->type = fade.type;
    out->color = ColorString::toColor(fade.color);
    out->on = fade.on;
}

// orig parseParticle 001c7dd0 (weak)
void parseParticle(std::shared_ptr<SceneData> scene, std::shared_ptr<ExScenarioData> scenario)
{
    const ExScenarioDataVisualParticle &particle = scenario->visual.particle;
    std::shared_ptr<ViewEffectData> effect = scene->view->effect;
    if (particle.type == kNullMarker) {
        effect->stopParticles = true;
        return;
    }
    if (particle.type == "none") return;
    for (size_t i = 0; i < particle.specs.size(); ++i) {
        ExVisualParticleSpec spec = particle.specs[i];
        spec.path = cachePath(particle.specs[i].path);
        effect->particles.push_back(spec);
    }
}

// orig parseEffect 001c9430 (weak)
void parseEffect(std::shared_ptr<SceneData> scene, std::shared_ptr<ExScenarioData> scenario)
{
    std::shared_ptr<ViewEffectData> effect = std::make_shared<ViewEffectData>();
    scene->view->effect = effect;
    effect->shake = scenario->visual.shakeType;
    parseManpu(scene, scenario);
    parseFade(effect->fade, scenario->visual.fade);
    effect->camera = scenario->visual.cameraType;
    parseParticle(scene, scenario);
    effect->endMark = scenario->visual.endMarkType;
}

}  // namespace

std::shared_ptr<SceneData> SceneDataParser::parseScene(std::shared_ptr<ExScenarioData> scenario,
                                                       std::shared_ptr<ExOffsetData> offsets)
{
    std::shared_ptr<SceneData> scene = std::make_shared<SceneData>();
    parseText(scene, scenario);
    parseSound(scene, scenario);
    scene->view = std::make_shared<ViewData>();
    parseBackground(scene, scenario);
    parsePersonArray(scene, scenario, offsets);
    parseEffect(scene, scenario);
    return scene;
}

std::vector<std::shared_ptr<SceneData>> SceneDataParser::parse(std::shared_ptr<ExStoryData> story,
                                                               std::shared_ptr<ExOffsetData> offsets)
{
    std::vector<std::shared_ptr<SceneData>> scenes;
    for (size_t i = 0; i < story->scenarioData.size(); ++i)
        scenes.push_back(parseScene(story->scenarioData[i], offsets));
    return scenes;
}
