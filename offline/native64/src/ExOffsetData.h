// ExOffsetData: the character sprite layout file (Application/cache/json/offset.json or the path SceneManager::start
// is given). For every character it lists which layers make up each perspective (drawing order) and where each
// layer image sits:
//   {"<app_id>": {"<character_id>": {
//        "order":  {"<perspective>": ["<layer name>", ...]},                       // back to front
//        "offset": {"<perspective>": {"<size s|m|l>": {"<layer name>": {"x": "12", "y": "-40"}}}}}}}
// x/y are integers written as strings, in pixels of the layer image relative to the sprite's centre (see
// OffsetLayerData::getOffsetPoint for how they become an anchor point).
//
// Header also holds ExOffsetDataOrderKey and ExOffsetDataPerspectiveKey (plain keys, compared lexicographically).
#ifndef EXOFFSETDATA_H
#define EXOFFSETDATA_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "cocos2d.h"
#include "picojson.h"

struct ExOffsetDataOrderKey {
    std::string appId;
    std::string characterId;
    std::string perspective;

    ExOffsetDataOrderKey() {}
    ExOffsetDataOrderKey(const std::string &app, const std::string &character, const std::string &persp)
        : appId(app), characterId(character), perspective(persp) {}
    ~ExOffsetDataOrderKey() {}
};
bool operator<(const ExOffsetDataOrderKey &a, const ExOffsetDataOrderKey &b);  // orig 001c1a46

struct ExOffsetDataPerspectiveKey {
    std::string appId;
    std::string characterId;
    std::string perspective;
    std::string size;   // "s" | "m" | "l" (ExScenarioFgSizeNames)
    std::string layer;  // layer name, e.g. "01_body", "03_eye", "05_mouth"

    ExOffsetDataPerspectiveKey() {}
    ExOffsetDataPerspectiveKey(const std::string &app, const std::string &character, const std::string &persp,
                               const std::string &sizeName, const std::string &layerName)
        : appId(app), characterId(character), perspective(persp), size(sizeName), layer(layerName) {}
    ExOffsetDataPerspectiveKey(const ExOffsetDataPerspectiveKey &) = default;  // orig 001d1efc
    ~ExOffsetDataPerspectiveKey() {}
};
bool operator<(const ExOffsetDataPerspectiveKey &a, const ExOffsetDataPerspectiveKey &b);  // orig 001c1a98

class ExOffsetData {
public:
    // Layer names of each (app, character, perspective), back to front.
    std::map<ExOffsetDataOrderKey, std::vector<std::string>> order;
    // Offset of each layer image, in pixels (see OffsetLayerData::getOffsetPoint).
    std::map<ExOffsetDataPerspectiveKey, cocos2d::CCPoint> offset;

    // Parses the whole offset file. Invalid JSON yields empty tables. orig 001d2460
    static std::shared_ptr<ExOffsetData> parse(const char *json);
    // Adds one character's "order" and "offset" objects to `data` and returns it. orig 001d20cc
    static std::shared_ptr<ExOffsetData> parseJsonCharacter(std::shared_ptr<ExOffsetData> data,
                                                            std::map<std::string, picojson::value> &json,
                                                            std::string &appId, std::string &characterId);
};

#endif
