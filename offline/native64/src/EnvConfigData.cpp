#include "EnvConfigData.h"

#include <algorithm>
#include <cstring>

#include "ResourceManager.h"

const std::string EnvConfigData::KEY_ENV_COMMON = "common";
const std::string EnvConfigData::KEY_ENV_POINTER = "__pointer__";

namespace {
// Field names of the server's environment list (orig: file-local statics next to "env.").
const std::string KEY_APP_NAME = "app_name";
const std::string KEY_USER_ID = "user_id";
const std::string ENV_FIELD_PREFIX = "env.";
}  // namespace

// orig 001c6164 (SwitchableConfigData::init runs once per constructor level, twice in total)
EnvConfigData::EnvConfigData() {
    init();
}

// orig 001c08d4
EnvConfigData::~EnvConfigData() {}

// orig 001d52d2
void EnvConfigData::clearData() {
    SwitchableConfigData::clearData();
    switching(SwitchableConfigData::getEnv());
}

// orig 001d530e
std::string EnvConfigData::getReadablePath() {
    return ResourceManager::getInstance()->getDocumentPath(ConfigData::getReadablePath());
}

// orig 001d533c
std::string EnvConfigData::getWritablePath() {
    return ResourceManager::getInstance()->getDynamicDocumentPath(ConfigData::getWritablePath());
}

// orig 001d5408
std::string EnvConfigData::getEnv() {
    if (m_root.is<picojson::object>()) {
        const picojson::object& root = m_root.get<picojson::object>();
        picojson::object::const_iterator it = root.find(KEY_ENV_POINTER);
        if (it != root.end() && it->second.is<std::string>()) {
            return it->second.get<std::string>();
        }
    }
    return SwitchableConfigData::getEnv();
}

// orig 001d58dc
EnvConfigData& EnvConfigData::switching(const std::string& env) {
    SwitchableConfigData::switching(env);
    rootObject()[KEY_ENV_POINTER] = picojson::value(SwitchableConfigData::getEnv());
    return *this;
}

// orig 001d5878
std::vector<std::string> EnvConfigData::getEnvList() {
    std::vector<std::string> list = SwitchableConfigData::getEnvList();
    list.erase(std::remove(list.begin(), list.end(), KEY_ENV_POINTER), list.end());
    list.erase(std::remove(list.begin(), list.end(), KEY_ENV_COMMON), list.end());
    return list;
}

// orig 001d5558
int EnvConfigData::getInt(const std::string& key, const int& defaultValue) {
    if (_hasEnvValue<double>(getEnv(), key)) {
        return SwitchableConfigData::getInt(key, defaultValue);
    }
    double value = static_cast<double>(defaultValue);
    lookupEnvValue(KEY_ENV_COMMON, key, value);
    return static_cast<int>(value);
}

// orig 001d55d8
double EnvConfigData::getDouble(const std::string& key, const double& defaultValue) {
    if (_hasEnvValue<double>(getEnv(), key)) {
        return SwitchableConfigData::getDouble(key, defaultValue);
    }
    double value = defaultValue;
    lookupEnvValue(KEY_ENV_COMMON, key, value);
    return value;
}

// orig 001d5650
std::string EnvConfigData::getString(const std::string& key, const std::string& defaultValue) {
    if (_hasEnvValue<std::string>(getEnv(), key)) {
        return SwitchableConfigData::getString(key, defaultValue);
    }
    std::string value = defaultValue;
    lookupEnvValue(KEY_ENV_COMMON, key, value);
    return value;
}

// orig 001d56dc
bool EnvConfigData::getBoolean(const std::string& key, const bool& defaultValue) {
    if (_hasEnvValue<bool>(getEnv(), key)) {
        return SwitchableConfigData::getBoolean(key, defaultValue);
    }
    bool value = defaultValue;
    lookupEnvValue(KEY_ENV_COMMON, key, value);
    return value;
}

// orig 001d5938
EnvConfigData& EnvConfigData::merge(EnvConfigData& other) {
    if (other.m_root.is<picojson::object>()) {
        picojson::object& mine = rootObject();
        const picojson::object& theirs = other.m_root.get<picojson::object>();
        for (picojson::object::const_iterator it = theirs.begin(); it != theirs.end(); ++it) {
            if (it->first.substr(0, 1) == "_") {
                continue;  // bookkeeping keys such as "__pointer__" are never imported
            }
            picojson::object::iterator existing = mine.find(it->first);
            if (existing == mine.end()) {
                mine[it->first] = it->second;
                continue;
            }
            // The original walks both values as objects unconditionally; skip pairs that are not objects.
            if (!existing->second.is<picojson::object>() || !it->second.is<picojson::object>()) {
                continue;
            }
            picojson::object& target = existing->second.get<picojson::object>();
            const picojson::object& source = it->second.get<picojson::object>();
            for (picojson::object::const_iterator field = source.begin(); field != source.end(); ++field) {
                target[field->first] = field->second;
            }
        }
    }
    mapping();
    return *this;
}

// orig 001d5a30
bool EnvConfigData::convertEnvListJson(const std::string& json, std::string& out) {
    if (json.empty()) {
        return false;
    }
    picojson::value parsed;
    std::string error;
    const char* begin = json.c_str();
    picojson::parse(parsed, begin, begin + strlen(begin), &error);
    if (!error.empty() || !parsed.is<picojson::array>()) {
        return false;
    }

    picojson::object converted;
    const picojson::array& entries = parsed.get<picojson::array>();
    for (picojson::array::const_iterator entry = entries.begin(); entry != entries.end(); ++entry) {
        if (!entry->is<picojson::object>()) {
            return false;
        }
        const picojson::object& fields = entry->get<picojson::object>();
        picojson::object::const_iterator appName = fields.find(KEY_APP_NAME);
        picojson::object::const_iterator userId = fields.find(KEY_USER_ID);
        if (appName == fields.end() || userId == fields.end()) {
            return false;
        }
        // GUESS: both are read as strings without a type check in the original; non-strings become "".
        std::string sectionName = (appName->second.is<std::string>() ? appName->second.get<std::string>() : "") +
                                  "." + (userId->second.is<std::string>() ? userId->second.get<std::string>() : "");

        picojson::object section;
        for (picojson::object::const_iterator field = fields.begin(); field != fields.end(); ++field) {
            const picojson::value& value = field->second;
            if (value.is<std::string>() || value.is<double>() || value.is<bool>()) {
                section[ENV_FIELD_PREFIX + field->first] = value;
            }
        }
        converted[sectionName] = picojson::value(section);
    }
    out = picojson::value(converted).serialize();
    return true;
}

// orig 001d5d14
bool EnvConfigData::injectEnvListJson(std::string json) {
    std::string converted;
    if (!convertEnvListJson(json, converted) || converted.empty()) {
        return false;
    }
    EnvConfigData incoming;
    incoming.fromString(converted);
    merge(incoming);
    return true;
}
