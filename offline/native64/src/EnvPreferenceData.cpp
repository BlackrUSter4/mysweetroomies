#include "EnvPreferenceData.h"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace maid {

const char* EnvPreferenceData::KEY_ENVIRONMENT = "__pointer__";
const char* EnvPreferenceData::KEY_VERSION = "__version__";
const char* EnvPreferenceData::KEY_UPDATE_AT = "__update_at__";

// orig 001da0f0
EnvPreferenceData& EnvPreferenceData::putEnvPreferenceData(const std::string& env, const SimplePreferenceData& data) {
    putObject(env, data.toString());
    return *this;
}

// orig 001da120
EnvPreferenceData& EnvPreferenceData::setEnv(const std::string& env) {
    putString(KEY_ENVIRONMENT, env);
    return *this;
}

// orig 001da15c
std::string EnvPreferenceData::getEnv() const {
    return getString(KEY_ENVIRONMENT, "default");
}

// orig 001da1bc
int EnvPreferenceData::getVersion() const {
    return getInt(KEY_VERSION, 0);
}

// orig 001da1f8
std::string EnvPreferenceData::getUpdateAt() const {
    return getString(KEY_UPDATE_AT, "");
}

// orig 001da258
EnvPreferenceData& EnvPreferenceData::updateTime() {
    time_t now = time(nullptr);
    struct tm local;
    localtime_r(&now, &local);

    std::ostringstream stamp;
    stamp << std::setfill('0') << std::setw(4) << (local.tm_year + 1900) << '-' << std::setw(2) << (local.tm_mon + 1)
          << '-' << std::setw(2) << local.tm_mday << ' ' << std::setw(2) << local.tm_hour << ':' << std::setw(2)
          << local.tm_min << ':' << std::setw(2) << local.tm_sec;
    putString(KEY_UPDATE_AT, stamp.str());
    return *this;
}

// orig 001da3b8
bool EnvPreferenceData::_getEnvPreferenceData(const std::string& env, SimplePreferenceData& out) const {
    if (!contains(env)) {
        return false;
    }
    out.fromString(getObject(env, "{}").c_str());
    return true;
}

// orig 001da418
bool EnvPreferenceData::getEnvPreferenceData(const std::string& env, SimplePreferenceData& out) const {
    _getEnvPreferenceData("common", out);
    SimplePreferenceData specific;
    bool found = _getEnvPreferenceData(env, specific);
    out.merge(specific);
    return found;
}

// orig 001da47c
std::string EnvPreferenceData::getGlobalString(const std::string& key, const std::string& defaultValue) const {
    SimplePreferenceData common;
    if (!getEnvPreferenceData("common", common)) {
        return defaultValue;
    }
    return common.getString(key, defaultValue);
}

// orig 001da4ec
EnvPreferenceData& EnvPreferenceData::setGlobalString(const std::string& key, const std::string& value) {
    SimplePreferenceData common;
    common.putString(key, value);
    putEnvPreferenceData("common", common);
    return *this;
}

}  // namespace maid
