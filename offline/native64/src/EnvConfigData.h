#ifndef ENVCONFIGDATA_H
#define ENVCONFIGDATA_H

#include "SwitchableConfigData.h"

// The layout of the game's json/app.json files: environments plus a shared "common" section and a persisted
// pointer to the selected environment.
//
//   { "__pointer__": "default",
//     "common":  { "zip_file_password": "...", ... },
//     "default": { "site_url": "http://...", ... } }
//
// Reads look in the current environment first and fall back to "common" (a value only counts when it has the
// requested JSON type). Writes go to the current environment. The file is read from
// ResourceManager::getDocumentPath(path) (device copy, else APK copy) and written to getDynamicDocumentPath(path).
class EnvConfigData : public SwitchableConfigData {
public:
    static const std::string KEY_ENV_COMMON;   // "common"
    static const std::string KEY_ENV_POINTER;  // "__pointer__"

    EnvConfigData();
    virtual ~EnvConfigData();

    virtual void clearData();  // root = {}, environment "default", "__pointer__" = "default"

    virtual int getInt(const std::string& key, const int& defaultValue);
    virtual bool getBoolean(const std::string& key, const bool& defaultValue);
    virtual double getDouble(const std::string& key, const double& defaultValue);
    virtual std::string getString(const std::string& key, const std::string& defaultValue);

    virtual std::string getReadablePath();
    virtual std::string getWritablePath();

    // Selects `env` and records it in "__pointer__" (so it survives commit()).
    virtual EnvConfigData& switching(const std::string& env);
    // "__pointer__" when present and a string, otherwise the in-memory selection.
    virtual std::string getEnv();
    // Environment names only ("__pointer__" and "common" removed).
    virtual std::vector<std::string> getEnvList();

    // Copies every section of `other` whose name does not start with '_' into this config: new sections are
    // added whole, existing ones are updated key by key. Then mapping().
    virtual EnvConfigData& merge(EnvConfigData& other);

    // Converts the server's environment list
    //   [ { "app_name": "roomies", "user_id": "dev1", "site_url": "...", ... }, ... ]
    // into sections keyed "<app_name>.<user_id>" whose string/number/bool fields are renamed "env.<field>".
    // Returns false (leaving `out` untouched) for empty input, JSON errors, a non-array, or an entry that is not an
    // object or lacks app_name/user_id.
    bool convertEnvListJson(const std::string& json, std::string& out);
    // convertEnvListJson() + merge() the result into this config. True when something was merged.
    bool injectEnvListJson(std::string json);
};

#endif
