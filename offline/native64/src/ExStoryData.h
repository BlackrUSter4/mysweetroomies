// ExStoryData: a whole story part as delivered by the server (Application/cache/json/<scenario>.json):
//   {"scenario_id": "<id>", "scenario_data": [ <line>, <line>, ... ]}
// Each <line> becomes one ExScenarioData (see ExScenarioData.h), in order.
#ifndef EXSTORYDATA_H
#define EXSTORYDATA_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ExScenarioData.h"

class ExStoryData {
public:
    std::string scenarioId;  // "scenario_id"
    // GUESS: a string->string map the original carries but never fills or reads (kept for layout parity).
    std::map<std::string, std::string> properties;
    std::vector<std::shared_ptr<ExScenarioData>> scenarioData;  // "scenario_data", in story order

    // Parses the story JSON text. Invalid JSON yields an empty story. orig 001d47ec
    static std::shared_ptr<ExStoryData> parse(const char *json);
};

#endif
