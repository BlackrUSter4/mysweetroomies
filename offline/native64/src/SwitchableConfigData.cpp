#include "SwitchableConfigData.h"

const std::string SwitchableConfigData::DEFAULT_ENVIRONMENT = "default";

SwitchableConfigData::SwitchableConfigData() {
    init();
}

// orig 001c089c
SwitchableConfigData::~SwitchableConfigData() {}

// orig 001d60a4
bool SwitchableConfigData::init() {
    m_env = DEFAULT_ENVIRONMENT;
    m_root = parse("{\"default\":{}}");
    initDefaultValue();
    return true;
}

// orig 001d6378
void SwitchableConfigData::clearData() {
    ConfigData::clearData();
    m_env = DEFAULT_ENVIRONMENT;
}

// orig 001d5e82
bool SwitchableConfigData::hasEnv(const std::string& env) {
    if (!m_root.is<picojson::object>()) {
        return false;
    }
    const picojson::object& root = m_root.get<picojson::object>();
    return root.find(env) != root.end();
}

// orig 001d546c (string), 001d5e96 (double): same shape for every type.
template <typename T>
bool SwitchableConfigData::lookupEnvValue(const std::string& env, const std::string& key, T& out) {
    if (!hasEnv(env)) {
        return false;
    }
    const picojson::value& section = m_root.get<picojson::object>().at(env);
    if (!section.is<picojson::object>()) {
        return false;
    }
    const picojson::object& values = section.get<picojson::object>();
    picojson::object::const_iterator it = values.find(key);
    if (it == values.end() || !it->second.is<T>()) {
        return false;
    }
    out = it->second.get<T>();
    return true;
}

// orig 001d54ca
template <typename T> bool SwitchableConfigData::_hasEnvValue(const std::string& env, const std::string& key) {
    T ignored;
    return lookupEnvValue(env, key, ignored);
}

template bool SwitchableConfigData::_hasEnvValue<double>(const std::string&, const std::string&);
template bool SwitchableConfigData::_hasEnvValue<bool>(const std::string&, const std::string&);
template bool SwitchableConfigData::_hasEnvValue<std::string>(const std::string&, const std::string&);
template bool SwitchableConfigData::lookupEnvValue<double>(const std::string&, const std::string&, double&);
template bool SwitchableConfigData::lookupEnvValue<bool>(const std::string&, const std::string&, bool&);
template bool SwitchableConfigData::lookupEnvValue<std::string>(const std::string&, const std::string&,
                                                                std::string&);

// orig 001d5dd0
std::string SwitchableConfigData::getEnv() {
    return m_env;
}

// orig 001d5dde
SwitchableConfigData& SwitchableConfigData::switching(const std::string& env) {
    m_env = env;
    mapping();
    return *this;
}

// orig 001d5df2
std::vector<std::string> SwitchableConfigData::getEnvList() {
    std::vector<std::string> list;
    if (m_root.is<picojson::object>()) {
        const picojson::object& root = m_root.get<picojson::object>();
        for (picojson::object::const_iterator it = root.begin(); it != root.end(); ++it) {
            list.push_back(it->first);
        }
    }
    return list;
}

// orig 001d6110
SwitchableConfigData& SwitchableConfigData::createEnv(const std::string& env) {
    rootObject()[env] = parse("{}");
    return *this;
}

// orig 001d5eda
int SwitchableConfigData::getInt(const std::string& key, const int& defaultValue) {
    double value = static_cast<double>(defaultValue);
    lookupEnvValue(getEnv(), key, value);
    return static_cast<int>(value);
}

// orig 001d5f52
double SwitchableConfigData::getDouble(const std::string& key, const double& defaultValue) {
    double value = defaultValue;
    lookupEnvValue(getEnv(), key, value);
    return value;
}

// orig 001d5fbe
bool SwitchableConfigData::getBoolean(const std::string& key, const bool& defaultValue) {
    bool value = defaultValue;
    lookupEnvValue(getEnv(), key, value);
    return value;
}

// orig 001d604e
std::string SwitchableConfigData::getString(const std::string& key, const std::string& defaultValue) {
    std::string value = defaultValue;
    lookupEnvValue(getEnv(), key, value);
    return value;
}

// orig 001d616c (_putEnvValue<double>) and the inlined string/bool variants
SwitchableConfigData& SwitchableConfigData::putEnvValue(const std::string& env, const std::string& key,
                                                        const picojson::value& value) {
    // std::map::at throws std::out_of_range for an unknown environment, as in the original.
    picojson::value& section = rootObject().at(env);
    if (!section.is<picojson::object>()) {
        // The original would treat a non-object as a map and corrupt memory; start an empty object instead.
        section = picojson::value(picojson::object());
    }
    section.get<picojson::object>()[key] = value;
    mapping();
    return *this;
}

// orig 001d61b2
SwitchableConfigData& SwitchableConfigData::putInt(const std::string& key, int value) {
    return putEnvValue(getEnv(), key, picojson::value(static_cast<double>(value)));
}

// orig 001d6210
SwitchableConfigData& SwitchableConfigData::putDouble(const std::string& key, double value) {
    return putEnvValue(getEnv(), key, picojson::value(value));
}

// orig 001d62fc
SwitchableConfigData& SwitchableConfigData::putBoolean(const std::string& key, bool value) {
    return putEnvValue(getEnv(), key, picojson::value(value));
}

// orig 001d6266
SwitchableConfigData& SwitchableConfigData::putString(const std::string& key, std::string value) {
    return putEnvValue(getEnv(), key, picojson::value(value));
}
