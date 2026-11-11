#ifndef MAID_MULTIPREFERENCEDATA_H
#define MAID_MULTIPREFERENCEDATA_H

#include "SimplePreferenceData.h"

namespace maid {

// A SimplePreferenceData whose top-level keys are sections, each a nested JSON object:
//
//   { "common": { "volume": 1 }, "<local>": { "volume": 0.5 } }
//
// "Global" accessors work on the KEY_GLOBAL ("common") section. "Local" getters read the named section and fall back
// to the global value when the key is absent there; local putters only write the named section. Missing sections
// read as empty. Type errors throw maid::JsonException like SimplePreferenceData.
class MultiPreferenceData : public SimplePreferenceData {
public:
    static const char* KEY_GLOBAL;  // "common"

    std::string getGlobalString(const std::string& key, const std::string& defaultValue) const;
    bool getGlobalBoolean(const std::string& key, bool defaultValue) const;
    double getGlobalDouble(const std::string& key, double defaultValue) const;
    int getGlobalInt(const std::string& key, int defaultValue) const;
    std::string getGlobalObject(const std::string& key, const std::string& defaultValue) const;

    MultiPreferenceData& putGlobalString(const std::string& key, const std::string& value);
    MultiPreferenceData& putGlobalBoolean(const std::string& key, bool value);
    MultiPreferenceData& putGlobalDouble(const std::string& key, double value);
    MultiPreferenceData& putGlobalInt(const std::string& key, int value);
    MultiPreferenceData& putGlobalObject(const std::string& key, const std::string& json);

    std::string getLocalString(const std::string& local, const std::string& key,
                               const std::string& defaultValue) const;
    bool getLocalBoolean(const std::string& local, const std::string& key, bool defaultValue) const;
    double getLocalDouble(const std::string& local, const std::string& key, double defaultValue) const;
    int getLocalInt(const std::string& local, const std::string& key, int defaultValue) const;
    std::string getLocalObject(const std::string& local, const std::string& key,
                               const std::string& defaultValue) const;

    MultiPreferenceData& putLocalString(const std::string& local, const std::string& key, const std::string& value);
    MultiPreferenceData& putLocalBoolean(const std::string& local, const std::string& key, bool value);
    MultiPreferenceData& putLocalDouble(const std::string& local, const std::string& key, double value);
    MultiPreferenceData& putLocalInt(const std::string& local, const std::string& key, int value);
    MultiPreferenceData& putLocalObject(const std::string& local, const std::string& key, const std::string& json);

private:
    SimplePreferenceData section(const std::string& name) const;
};

}  // namespace maid

#endif
