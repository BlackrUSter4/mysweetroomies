#ifndef SWITCHABLECONFIGDATA_H
#define SWITCHABLECONFIGDATA_H

#include <vector>

#include "ConfigData.h"

// A ConfigData whose root holds one object per environment and reads/writes the current one:
//
//   { "default": { "site_url": "http://..." }, "develop": {...} }
//
// get* return the current environment's value when it exists with the right type, else the default.
// put* write into the current environment's object (std::out_of_range when that environment does not exist) and
// re-run mapping(). switching() selects another environment (it does not create it; see createEnv()).
class SwitchableConfigData : public ConfigData {
public:
    static const std::string DEFAULT_ENVIRONMENT;  // "default"

    SwitchableConfigData();  // root = {"default":{}}, environment "default"
    virtual ~SwitchableConfigData();

    virtual bool init();        // as the constructor
    virtual void clearData();   // root = {}, environment back to "default"

    virtual int getInt(const std::string& key, const int& defaultValue);
    virtual bool getBoolean(const std::string& key, const bool& defaultValue);
    virtual double getDouble(const std::string& key, const double& defaultValue);
    virtual std::string getString(const std::string& key, const std::string& defaultValue);

    virtual SwitchableConfigData& putInt(const std::string& key, int value);
    virtual SwitchableConfigData& putBoolean(const std::string& key, bool value);
    virtual SwitchableConfigData& putDouble(const std::string& key, double value);
    virtual SwitchableConfigData& putString(const std::string& key, std::string value);

    virtual SwitchableConfigData& switching(const std::string& env);  // select `env`, then mapping()
    virtual std::string getEnv();                                     // current environment name
    virtual std::vector<std::string> getEnvList();                    // every top-level key, sorted

    bool hasEnv(const std::string& env);                 // top-level key exists
    SwitchableConfigData& createEnv(const std::string& env);  // root[env] = {}

protected:
    // `env`'s value for `key` if root[env] is an object holding a T there.
    template <typename T> bool lookupEnvValue(const std::string& env, const std::string& key, T& out);
    template <typename T> bool _hasEnvValue(const std::string& env, const std::string& key);
    // root.at(env)[key] = value, then mapping().
    SwitchableConfigData& putEnvValue(const std::string& env, const std::string& key, const picojson::value& value);

    std::string m_env;
};

#endif
