#ifndef MAID_ENVPREFERENCEDATA_H
#define MAID_ENVPREFERENCEDATA_H

#include "SimplePreferenceData.h"

namespace maid {

// Per-environment preferences: top-level keys are environment names (plus "common") holding nested objects, and a
// few bookkeeping keys:
//
//   { "__pointer__": "default", "__version__": 3, "__update_at__": "2017-04-01 12:00:00",
//     "common": {...}, "default": {...} }
class EnvPreferenceData : public SimplePreferenceData {
public:
    static const char* KEY_ENVIRONMENT;  // "__pointer__"   current environment name
    static const char* KEY_VERSION;      // "__version__"   int
    static const char* KEY_UPDATE_AT;    // "__update_at__" "YYYY-MM-DD HH:MM:SS" local time

    EnvPreferenceData& setEnv(const std::string& env);
    std::string getEnv() const;          // defaults to "default"
    int getVersion() const;              // defaults to 0
    std::string getUpdateAt() const;     // defaults to ""
    EnvPreferenceData& updateTime();     // stamps KEY_UPDATE_AT with the current local time

    // Stores `data` as the section `env` (replacing it).
    EnvPreferenceData& putEnvPreferenceData(const std::string& env, const SimplePreferenceData& data);
    // Loads "common" into `out`, then merges section `env` over it. Returns whether `env` existed.
    bool getEnvPreferenceData(const std::string& env, SimplePreferenceData& out) const;

    // Reads `key` from the "common" section (default when the section is missing).
    std::string getGlobalString(const std::string& key, const std::string& defaultValue) const;
    // Replaces the whole "common" section with { key: value } (the original does not keep other keys).
    EnvPreferenceData& setGlobalString(const std::string& key, const std::string& value);

private:
    bool _getEnvPreferenceData(const std::string& env, SimplePreferenceData& out) const;
};

}  // namespace maid

#endif
