#include "MultiPreferenceData.h"

namespace maid {

const char* MultiPreferenceData::KEY_GLOBAL = "common";

SimplePreferenceData MultiPreferenceData::section(const std::string& name) const {
    return SimplePreferenceData(getObject(name, "{}").c_str());
}

// --- global section (orig 001da548 .. 001dabxx) ---

std::string MultiPreferenceData::getGlobalString(const std::string& key, const std::string& defaultValue) const {
    return section(KEY_GLOBAL).getString(key, defaultValue);
}

bool MultiPreferenceData::getGlobalBoolean(const std::string& key, bool defaultValue) const {
    return section(KEY_GLOBAL).getBoolean(key, defaultValue);
}

double MultiPreferenceData::getGlobalDouble(const std::string& key, double defaultValue) const {
    return section(KEY_GLOBAL).getDouble(key, defaultValue);
}

int MultiPreferenceData::getGlobalInt(const std::string& key, int defaultValue) const {
    return section(KEY_GLOBAL).getInt(key, defaultValue);
}

std::string MultiPreferenceData::getGlobalObject(const std::string& key, const std::string& defaultValue) const {
    return section(KEY_GLOBAL).getObject(key, defaultValue);
}

MultiPreferenceData& MultiPreferenceData::putGlobalString(const std::string& key, const std::string& value) {
    SimplePreferenceData global = section(KEY_GLOBAL);
    global.putString(key, value);
    putObject(KEY_GLOBAL, global.toString());
    return *this;
}

MultiPreferenceData& MultiPreferenceData::putGlobalBoolean(const std::string& key, bool value) {
    SimplePreferenceData global = section(KEY_GLOBAL);
    global.putBoolean(key, value);
    putObject(KEY_GLOBAL, global.toString());
    return *this;
}

MultiPreferenceData& MultiPreferenceData::putGlobalDouble(const std::string& key, double value) {
    SimplePreferenceData global = section(KEY_GLOBAL);
    global.putDouble(key, value);
    putObject(KEY_GLOBAL, global.toString());
    return *this;
}

MultiPreferenceData& MultiPreferenceData::putGlobalInt(const std::string& key, int value) {
    SimplePreferenceData global = section(KEY_GLOBAL);
    global.putInt(key, value);
    putObject(KEY_GLOBAL, global.toString());
    return *this;
}

MultiPreferenceData& MultiPreferenceData::putGlobalObject(const std::string& key, const std::string& json) {
    SimplePreferenceData global = section(KEY_GLOBAL);
    global.putObject(key, json);
    putObject(KEY_GLOBAL, global.toString());
    return *this;
}

// --- named sections, falling back to the global one on read (orig 001dac54 .. 001db1xx) ---

std::string MultiPreferenceData::getLocalString(const std::string& local, const std::string& key,
                                                const std::string& defaultValue) const {
    SimplePreferenceData data = section(local);
    return data.contains(key) ? data.getString(key, defaultValue) : getGlobalString(key, defaultValue);
}

bool MultiPreferenceData::getLocalBoolean(const std::string& local, const std::string& key, bool defaultValue) const {
    SimplePreferenceData data = section(local);
    return data.contains(key) ? data.getBoolean(key, defaultValue) : getGlobalBoolean(key, defaultValue);
}

double MultiPreferenceData::getLocalDouble(const std::string& local, const std::string& key,
                                           double defaultValue) const {
    SimplePreferenceData data = section(local);
    return data.contains(key) ? data.getDouble(key, defaultValue) : getGlobalDouble(key, defaultValue);
}

int MultiPreferenceData::getLocalInt(const std::string& local, const std::string& key, int defaultValue) const {
    SimplePreferenceData data = section(local);
    return data.contains(key) ? data.getInt(key, defaultValue) : getGlobalInt(key, defaultValue);
}

std::string MultiPreferenceData::getLocalObject(const std::string& local, const std::string& key,
                                                const std::string& defaultValue) const {
    SimplePreferenceData data = section(local);
    return data.contains(key) ? data.getObject(key, defaultValue) : getGlobalObject(key, defaultValue);
}

MultiPreferenceData& MultiPreferenceData::putLocalString(const std::string& local, const std::string& key,
                                                         const std::string& value) {
    SimplePreferenceData data = section(local);
    data.putString(key, value);
    putObject(local, data.toString());
    return *this;
}

MultiPreferenceData& MultiPreferenceData::putLocalBoolean(const std::string& local, const std::string& key,
                                                          bool value) {
    SimplePreferenceData data = section(local);
    data.putBoolean(key, value);
    putObject(local, data.toString());
    return *this;
}

MultiPreferenceData& MultiPreferenceData::putLocalDouble(const std::string& local, const std::string& key,
                                                         double value) {
    SimplePreferenceData data = section(local);
    data.putDouble(key, value);
    putObject(local, data.toString());
    return *this;
}

MultiPreferenceData& MultiPreferenceData::putLocalInt(const std::string& local, const std::string& key, int value) {
    SimplePreferenceData data = section(local);
    data.putInt(key, value);
    putObject(local, data.toString());
    return *this;
}

MultiPreferenceData& MultiPreferenceData::putLocalObject(const std::string& local, const std::string& key,
                                                         const std::string& json) {
    SimplePreferenceData data = section(local);
    data.putObject(key, json);
    putObject(local, data.toString());
    return *this;
}

}  // namespace maid
