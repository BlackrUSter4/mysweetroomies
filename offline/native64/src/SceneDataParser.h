// SceneDataParser: turns the raw story (ExStoryData) plus the sprite layout file (ExOffsetData) into the SceneData
// list SceneManager plays. Resolves every file name to a resource-cache path, builds character layers and their
// blink/lip-sync animations, and pre-renders filtered images (FilterData::generateFilteredImage).
#ifndef SCENEDATAPARSER_H
#define SCENEDATAPARSER_H

#include <memory>
#include <vector>

#include "ExOffsetData.h"
#include "ExScenarioData.h"
#include "ExStoryData.h"
#include "SceneData.h"

class SceneDataParser {
public:
    // One SceneData per ExStoryData::scenarioData entry, same order. orig 001d9e58
    static std::vector<std::shared_ptr<SceneData>> parse(std::shared_ptr<ExStoryData> story,
                                                         std::shared_ptr<ExOffsetData> offsets);
    // One line. Fills text, sound, view (background, persons, effect) in that order; persons need sound first
    // because the voice file drives the mouth animation. orig 001d9c58
    static std::shared_ptr<SceneData> parseScene(std::shared_ptr<ExScenarioData> scenario,
                                                 std::shared_ptr<ExOffsetData> offsets);
};

#endif
