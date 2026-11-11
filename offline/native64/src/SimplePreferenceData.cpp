#include "SimplePreferenceData.h"

#include <cstring>
#include <sstream>

#include "JsonException.h"

namespace maid {

// orig 001db244
SimplePreferenceData::SimplePreferenceData() : m_json("{}") {}

// orig 001dbca4
SimplePreferenceData::SimplePreferenceData(const char* json) {
    fromString(json);
}

// orig 001db1d8
SimplePreferenceData::~SimplePreferenceData() {}

// orig 001db328
void SimplePreferenceData::parse(const char* json, picojson::value& out) const {
    std::string error;
    picojson::parse(out, json, json + strlen(json), &error);
    if (!error.empty()) {
        throw JsonException(error + " : " + json);
    }
}

// orig 001db4dc
void SimplePreferenceData::parseObject(const char* json, std::map<std::string, picojson::value>& out) const {
    picojson::value value;
    parse(json, value);
    if (!value.is<picojson::object>()) {
        throw JsonException("json is not type of object");
    }
    out = value.get<picojson::object>();
}

void SimplePreferenceData::storeObject(const picojson::object& object) {
    m_json = picojson::value(object).serialize();
}

// orig 001dbc5e
SimplePreferenceData& SimplePreferenceData::fromString(const char* json) {
    picojson::object validated;
    parseObject(json, validated);
    m_json = json;
    return *this;
}

// orig 001db284
std::string SimplePreferenceData::toString() const {
    return m_json;
}

// orig 001db5b8
bool SimplePreferenceData::contains(const std::string& key) const {
    picojson::object object;
    parseObject(m_json.c_str(), object);
    return object.find(key) != object.end();
}

// orig 001db5f8
bool SimplePreferenceData::getBoolean(const std::string& key, bool defaultValue) const {
    picojson::object object;
    parseObject(m_json.c_str(), object);
    picojson::object::const_iterator it = object.find(key);
    if (it == object.end()) {
        return defaultValue;
    }
    if (!it->second.is<bool>()) {
        throw JsonException("obj[" + key + "] value is not bool");
    }
    return it->second.get<bool>();
}

// orig 001db6bc
std::string SimplePreferenceData::getString(const std::string& key, const std::string& defaultValue) const {
    picojson::object object;
    parseObject(m_json.c_str(), object);
    picojson::object::const_iterator it = object.find(key);
    if (it == object.end()) {
        return defaultValue;
    }
    if (!it->second.is<std::string>()) {
        throw JsonException("obj[" + key + "] value is not string");
    }
    return it->second.get<std::string>();
}

// orig 001db78c
int SimplePreferenceData::getInt(const std::string& key, int defaultValue) const {
    picojson::object object;
    parseObject(m_json.c_str(), object);
    picojson::object::const_iterator it = object.find(key);
    if (it == object.end()) {
        return defaultValue;
    }
    if (!it->second.is<double>()) {
        // GUESS: the original has only three "] value is not ..." strings; getInt shares getDouble's.
        throw JsonException("obj[" + key + "] value is not double");
    }
    return static_cast<int>(it->second.get<double>());
}

// orig 001db858
double SimplePreferenceData::getDouble(const std::string& key, double defaultValue) const {
    picojson::object object;
    parseObject(m_json.c_str(), object);
    picojson::object::const_iterator it = object.find(key);
    if (it == object.end()) {
        return defaultValue;
    }
    if (!it->second.is<double>()) {
        throw JsonException("obj[" + key + "] value is not double");
    }
    return it->second.get<double>();
}

// orig 001db928
std::string SimplePreferenceData::getObject(const std::string& key, const std::string& defaultValue) const {
    picojson::object object;
    parseObject(m_json.c_str(), object);
    picojson::object::const_iterator it = object.find(key);
    if (it == object.end()) {
        return defaultValue;
    }
    const picojson::value& value = it->second;
    if (value.is<std::string>()) {
        return value.get<std::string>();
    }
    if (value.is<bool>()) {
        return value.get<bool>() ? "true" : "false";
    }
    if (value.is<double>()) {
        std::ostringstream text;
        text << value.get<double>();
        return text.str();
    }
    if (value.is<picojson::array>() || value.is<picojson::object>()) {
        return value.serialize();
    }
    return "";
}

// orig 001dba04
SimplePreferenceData& SimplePreferenceData::putBoolean(const std::string& key, bool value) {
    picojson::object object;
    parseObject(m_json.c_str(), object);
    object[key] = picojson::value(value);
    storeObject(object);
    return *this;
}

// orig 001dba8e
SimplePreferenceData& SimplePreferenceData::putString(const std::string& key, const std::string& value) {
    picojson::object object;
    parseObject(m_json.c_str(), object);
    object[key] = picojson::value(value);
    storeObject(object);
    return *this;
}

// orig 001dbb36
SimplePreferenceData& SimplePreferenceData::putInt(const std::string& key, int value) {
    picojson::object object;
    parseObject(m_json.c_str(), object);
    object[key] = picojson::value(static_cast<double>(value));
    storeObject(object);
    return *this;
}

// orig 001dbbcc
SimplePreferenceData& SimplePreferenceData::putDouble(const std::string& key, double value) {
    picojson::object object;
    parseObject(m_json.c_str(), object);
    object[key] = picojson::value(value);
    storeObject(object);
    return *this;
}

// orig 001dbe0c
SimplePreferenceData& SimplePreferenceData::putObject(const std::string& key, const std::string& json) {
    picojson::value parsed;
    parse(json.c_str(), parsed);
    picojson::object object;
    parseObject(m_json.c_str(), object);
    object[key] = parsed;
    storeObject(object);
    return *this;
}

// orig 001dbce4
SimplePreferenceData& SimplePreferenceData::foreach(
    std::function<void(const std::string&, SimplePreferenceData&)> callback) {
    picojson::object object;
    parseObject(m_json.c_str(), object);
    for (picojson::object::const_iterator it = object.begin(); it != object.end(); ++it) {
        callback(it->first, *this);
    }
    return *this;
}

// orig 001dbd3a
SimplePreferenceData& SimplePreferenceData::merge(const SimplePreferenceData& other) {
    picojson::object mine;
    parseObject(m_json.c_str(), mine);
    std::string otherJson = other.toString();
    picojson::object theirs;
    parseObject(otherJson.c_str(), theirs);
    for (picojson::object::const_iterator it = theirs.begin(); it != theirs.end(); ++it) {
        if (!it->second.is<picojson::null>()) {
            mine[it->first] = it->second;
        }
    }
    storeObject(mine);
    return *this;
}

}  // namespace maid
