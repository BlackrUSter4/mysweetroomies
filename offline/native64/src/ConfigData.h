#ifndef CONFIGDATA_H
#define CONFIGDATA_H

#include <string>

#include "picojson.h"

// A JSON object held in memory and persisted to a file.
//
// The root is a picojson object; get*/put* read and write its top-level keys (numbers are JSON doubles, getInt
// truncates). Nothing touches the disk until commit()/save(). Subclasses decide where files are read from and
// written to (getReadablePath/getWritablePath) and may react to every reload (initLoadValue).
//
// Typical use:  config.putString("k", "v").commit();
class ConfigData {
public:
    ConfigData();  // empty object; calls init()
    virtual ~ConfigData();

    virtual bool init();        // root = {}; then initDefaultValue()
    virtual void clearData();   // root = {}

    // A missing key, a JSON null or (deviation, see .cpp) a value of another type returns `defaultValue`.
    virtual int getInt(const std::string& key, const int& defaultValue);
    virtual bool getBoolean(const std::string& key, const bool& defaultValue);
    virtual double getDouble(const std::string& key, const double& defaultValue);
    virtual std::string getString(const std::string& key, const std::string& defaultValue);

    virtual ConfigData& putInt(const std::string& key, int value);
    virtual ConfigData& putBoolean(const std::string& key, bool value);
    virtual ConfigData& putDouble(const std::string& key, double value);
    virtual ConfigData& putString(const std::string& key, std::string value);

    virtual ConfigData& setPath(const std::string& path);  // remembered for refetch()/commit()
    virtual std::string getReadablePath();                 // base: the path as given
    virtual std::string getWritablePath();                 // base: the path as given

    // clearData(), setPath(path), parse the file at getReadablePath() (via ResourceManager::slurp), mapping().
    ConfigData& load(const std::string& path);
    ConfigData& refetch();                       // load(current path)
    // setPath(path), then write the serialized root to getWritablePath() (creating parent directories).
    ConfigData& save(const std::string& path);
    ConfigData& commit();                        // save(current path)
    ConfigData& clearFile();                     // delete the file at getWritablePath()

    // clearData(), parse `json`, mapping().
    ConfigData& fromString(const std::string& json);
    std::string toString();                      // serialized root

    // Parses JSON; returns null (or whatever parsed so far) on a syntax error, like the original.
    static picojson::value parse(const std::string& json);

protected:
    virtual void initDefaultValue();                       // hook for init(); base does nothing
    virtual void initLoadValue(const picojson::value& root);  // hook after every (re)load; base does nothing
    ConfigData& mapping();                                 // initLoadValue(root)
    picojson::object& rootObject();                        // the root as an object (null -> {})

    picojson::value m_root;
    std::string m_path;

private:
    // Typed lookup shared by the getters: the value under `key` when it exists and has type T.
    template <typename T> bool lookup(const std::string& key, T& out) const;
};

#endif
