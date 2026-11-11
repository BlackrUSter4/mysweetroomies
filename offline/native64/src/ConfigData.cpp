#include "ConfigData.h"

#include <cstring>
#include <fstream>

#include "ResourceManager.h"

// orig 001d5150
ConfigData::ConfigData() {
    init();
}

// orig 001d4dcc
ConfigData::~ConfigData() {}

// orig 001d5038
picojson::value ConfigData::parse(const std::string& json) {
    picojson::value value;
    std::string error;
    const char* begin = json.c_str();
    picojson::parse(value, begin, begin + strlen(begin), &error);
    // The original builds picojson's error text and drops it: a broken file simply yields a null/partial root.
    return value;
}

// orig 001d50f4
bool ConfigData::init() {
    m_root = parse("{}");
    initDefaultValue();
    return true;
}

// orig 001d4c28
void ConfigData::initDefaultValue() {}

// orig 001c611e
void ConfigData::initLoadValue(const picojson::value& root) {
    (void)root;
}

// orig 001d4cb6
ConfigData& ConfigData::mapping() {
    initLoadValue(m_root);
    return *this;
}

// orig 001d5190
void ConfigData::clearData() {
    m_root = parse("{}");
}

// orig 001d4c2a
ConfigData& ConfigData::setPath(const std::string& path) {
    m_path = path;
    return *this;
}

// orig 001d4c4e
std::string ConfigData::getReadablePath() {
    return m_path;
}

// orig 001d4c40
std::string ConfigData::getWritablePath() {
    return m_path;
}

// orig 001d51e4
ConfigData& ConfigData::load(const std::string& path) {
    clearData();
    setPath(path);
    std::string text = ResourceManager::getInstance()->slurp(getReadablePath());
    m_root = parse(text);
    mapping();
    return *this;
}

// orig 001d5286
ConfigData& ConfigData::refetch() {
    std::string path = m_path;
    return load(path);
}

// orig 001d4cf0
ConfigData& ConfigData::save(const std::string& path) {
    setPath(path);
    std::string json = m_root.serialize();
    std::string writablePath = getWritablePath();

    ResourceManager* resources = ResourceManager::getInstance();
    std::string parent = resources->getParentDir(writablePath);
    if (parent != "" && !resources->isExistDir(parent)) {
        resources->mkdirs(parent);
    }
    std::ofstream out(writablePath.c_str(), std::ios::out);
    out << json;
    out.flush();
    out.close();
    return *this;
}

// orig 001d4db0
ConfigData& ConfigData::commit() {
    std::string path = m_path;
    return save(path);
}

// orig 001d4c6a
ConfigData& ConfigData::clearFile() {
    if (!getWritablePath().empty()) {
        ResourceManager::getInstance()->removeFile(getWritablePath());
    }
    return *this;
}

// orig 001d5296
ConfigData& ConfigData::fromString(const std::string& json) {
    clearData();
    m_root = parse(json);
    mapping();
    return *this;
}

// orig 001d4dbc
std::string ConfigData::toString() {
    return m_root.serialize();
}

// The original reads the union of any non-null value without checking its type (garbage for a mismatch, a crash
// for getString on a non-string). Here a mismatch falls back to the default instead.
template <typename T> bool ConfigData::lookup(const std::string& key, T& out) const {
    if (!m_root.is<picojson::object>()) {
        return false;
    }
    const picojson::object& object = m_root.get<picojson::object>();
    picojson::object::const_iterator it = object.find(key);
    if (it == object.end() || !it->second.is<T>()) {
        return false;
    }
    out = it->second.get<T>();
    return true;
}

// orig 001d4e60
int ConfigData::getInt(const std::string& key, const int& defaultValue) {
    double value = static_cast<double>(defaultValue);
    lookup(key, value);
    return static_cast<int>(value);
}

// orig 001d4e80
double ConfigData::getDouble(const std::string& key, const double& defaultValue) {
    double value = defaultValue;
    lookup(key, value);
    return value;
}

// orig 001d4e88
bool ConfigData::getBoolean(const std::string& key, const bool& defaultValue) {
    bool value = defaultValue;
    lookup(key, value);
    return value;
}

// orig 001d4eba
std::string ConfigData::getString(const std::string& key, const std::string& defaultValue) {
    std::string value = defaultValue;
    lookup(key, value);
    return value;
}

// A root that failed to parse is null; the original would dereference it here, this port starts a fresh object.
picojson::object& ConfigData::rootObject() {
    if (!m_root.is<picojson::object>()) {
        m_root = picojson::value(picojson::object());
    }
    return m_root.get<picojson::object>();
}

// orig 001d4f7e
ConfigData& ConfigData::putInt(const std::string& key, int value) {
    rootObject()[key] = picojson::value(static_cast<double>(value));
    return *this;
}

// orig 001d4f98 / 001d4f48
ConfigData& ConfigData::putDouble(const std::string& key, double value) {
    rootObject()[key] = picojson::value(value);
    return *this;
}

// orig 001d5006
ConfigData& ConfigData::putBoolean(const std::string& key, bool value) {
    rootObject()[key] = picojson::value(value);
    return *this;
}

// orig 001d4fa0
ConfigData& ConfigData::putString(const std::string& key, std::string value) {
    rootObject()[key] = picojson::value(value);
    return *this;
}
