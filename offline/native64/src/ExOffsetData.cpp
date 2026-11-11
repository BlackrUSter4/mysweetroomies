#include "ExOffsetData.h"

#include <cstdlib>
#include <cstring>
#include <tuple>

USING_NS_CC;

bool operator<(const ExOffsetDataOrderKey &a, const ExOffsetDataOrderKey &b)
{
    return std::tie(a.appId, a.characterId, a.perspective) < std::tie(b.appId, b.characterId, b.perspective);
}

bool operator<(const ExOffsetDataPerspectiveKey &a, const ExOffsetDataPerspectiveKey &b)
{
    return std::tie(a.appId, a.characterId, a.perspective, a.size, a.layer) <
           std::tie(b.appId, b.characterId, b.perspective, b.size, b.layer);
}

namespace {

// x/y are strings holding integers ("12"); the original atoi'd them. A plain JSON number is accepted as well.
int jsonInt(const picojson::object &json, const char *key)
{
    picojson::object::const_iterator it = json.find(key);
    if (it == json.end()) return 0;
    if (it->second.is<std::string>()) return atoi(it->second.get<std::string>().c_str());
    if (it->second.is<double>()) return static_cast<int>(it->second.get<double>());
    return 0;
}

}  // namespace

std::shared_ptr<ExOffsetData> ExOffsetData::parseJsonCharacter(std::shared_ptr<ExOffsetData> data,
                                                               std::map<std::string, picojson::value> &json,
                                                               std::string &appId, std::string &characterId)
{
    picojson::object::iterator order = json.find("order");
    if (order != json.end() && order->second.is<picojson::object>()) {
        picojson::object &perspectives = order->second.get<picojson::object>();
        for (picojson::object::iterator p = perspectives.begin(); p != perspectives.end(); ++p) {
            std::vector<std::string> layers;
            if (p->second.is<picojson::array>()) {
                picojson::array &names = p->second.get<picojson::array>();
                for (picojson::array::iterator n = names.begin(); n != names.end(); ++n)
                    layers.push_back(n->is<std::string>() ? n->get<std::string>() : std::string());
            }
            // map::insert: a repeated key keeps the first list, as in the original.
            data->order.insert(std::make_pair(ExOffsetDataOrderKey(appId, characterId, p->first), layers));
        }
    }

    picojson::object::iterator offset = json.find("offset");
    if (offset != json.end() && offset->second.is<picojson::object>()) {
        picojson::object &perspectives = offset->second.get<picojson::object>();
        for (picojson::object::iterator p = perspectives.begin(); p != perspectives.end(); ++p) {
            if (!p->second.is<picojson::object>()) continue;
            picojson::object &sizes = p->second.get<picojson::object>();
            for (picojson::object::iterator s = sizes.begin(); s != sizes.end(); ++s) {
                if (!s->second.is<picojson::object>()) continue;
                picojson::object &layers = s->second.get<picojson::object>();
                for (picojson::object::iterator l = layers.begin(); l != layers.end(); ++l) {
                    if (!l->second.is<picojson::object>()) continue;
                    const picojson::object &xy = l->second.get<picojson::object>();
                    CCPoint point((float)jsonInt(xy, "x"), (float)jsonInt(xy, "y"));
                    data->offset[ExOffsetDataPerspectiveKey(appId, characterId, p->first, s->first, l->first)] =
                        point;
                }
            }
        }
    }
    return data;
}

std::shared_ptr<ExOffsetData> ExOffsetData::parse(const char *json)
{
    std::shared_ptr<ExOffsetData> data = std::make_shared<ExOffsetData>();

    picojson::value root;
    std::string error;
    picojson::parse(root, json, json + strlen(json), &error);
    // The original built a "syntax error at line N near: ..." message here and then ignored it.
    if (!root.is<picojson::object>()) return data;

    picojson::object &apps = root.get<picojson::object>();
    for (picojson::object::iterator app = apps.begin(); app != apps.end(); ++app) {
        if (!app->second.is<picojson::object>()) continue;
        std::string appId = app->first;
        picojson::object &characters = app->second.get<picojson::object>();
        for (picojson::object::iterator character = characters.begin(); character != characters.end(); ++character) {
            if (!character->second.is<picojson::object>()) continue;
            std::string characterId = character->first;
            parseJsonCharacter(data, character->second.get<picojson::object>(), appId, characterId);
        }
    }
    return data;
}
