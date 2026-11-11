#include "ExStoryData.h"

#include <cstring>

std::shared_ptr<ExStoryData> ExStoryData::parse(const char *json)
{
    std::shared_ptr<ExStoryData> story = std::make_shared<ExStoryData>();

    picojson::value root;
    std::string error;
    picojson::parse(root, json, json + strlen(json), &error);
    // The original built a "syntax error at line N near: ..." message here and then ignored it.
    if (!root.is<picojson::object>()) return story;
    picojson::object &object = root.get<picojson::object>();

    picojson::object::iterator id = object.find("scenario_id");
    if (id != object.end() && id->second.is<std::string>()) story->scenarioId = id->second.get<std::string>();

    picojson::object::iterator lines = object.find("scenario_data");
    if (lines != object.end() && lines->second.is<picojson::array>()) {
        picojson::array &list = lines->second.get<picojson::array>();
        for (picojson::array::iterator it = list.begin(); it != list.end(); ++it) {
            if (!it->is<picojson::object>()) continue;  // the original assumed objects
            story->scenarioData.push_back(ExScenarioData::parseJson(it->get<picojson::object>()));
        }
    }
    return story;
}
