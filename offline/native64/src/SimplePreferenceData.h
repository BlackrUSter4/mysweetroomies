#ifndef MAID_SIMPLEPREFERENCEDATA_H
#define MAID_SIMPLEPREFERENCEDATA_H

#include <functional>
#include <map>
#include <string>

#include "picojson.h"

namespace maid {

// A flat key/value store kept as a serialized JSON object string ("{}" when empty).
//
// Every accessor re-parses the string, so the object is cheap to copy and always serializable. Getters return the
// default when the key is missing and throw maid::JsonException when the key holds the wrong JSON type
// ("obj[<key>] value is not bool|string|double"). Numbers are stored as JSON doubles (getInt truncates).
// Any method that parses throws JsonException if the stored/passed text is not a JSON object.
class SimplePreferenceData {
public:
    SimplePreferenceData();                   // "{}"
    explicit SimplePreferenceData(const char* json);  // validates via fromString()
    virtual ~SimplePreferenceData();

    // Replaces the content with `json` (must be a JSON object).
    SimplePreferenceData& fromString(const char* json);
    std::string toString() const;

    bool contains(const std::string& key) const;

    bool getBoolean(const std::string& key, bool defaultValue) const;
    std::string getString(const std::string& key, const std::string& defaultValue) const;
    int getInt(const std::string& key, int defaultValue) const;
    double getDouble(const std::string& key, double defaultValue) const;
    // Any type, as text: strings verbatim, "true"/"false", numbers via ostream, arrays/objects serialized,
    // null -> "".
    std::string getObject(const std::string& key, const std::string& defaultValue) const;

    SimplePreferenceData& putBoolean(const std::string& key, bool value);
    SimplePreferenceData& putString(const std::string& key, const std::string& value);
    SimplePreferenceData& putInt(const std::string& key, int value);
    SimplePreferenceData& putDouble(const std::string& key, double value);
    // Stores the parsed JSON value of `json` (any JSON type) under `key`.
    SimplePreferenceData& putObject(const std::string& key, const std::string& json);

    // Calls `callback(key, *this)` once per top-level key. (The original passes the whole store, not the value.)
    SimplePreferenceData& foreach(std::function<void(const std::string&, SimplePreferenceData&)> callback);

    // Copies every non-null top-level value of `other` over this store.
    SimplePreferenceData& merge(const SimplePreferenceData& other);

protected:
    void parse(const char* json, picojson::value& out) const;
    void parseObject(const char* json, std::map<std::string, picojson::value>& out) const;

private:
    void storeObject(const picojson::object& object);

    std::string m_json;
};

}  // namespace maid

#endif
