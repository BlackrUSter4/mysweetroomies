#ifndef APPCONFIGDATA_H
#define APPCONFIGDATA_H

#include "EnvConfigData.h"

// The app's settings singleton (server URLs, current story id, sound volumes saved by other classes, ...).
//
// On first use it loads Application/document/json/app.json (or the device copy under the writable path once one
// has been saved), merges json/product/app.json and json/product/android/app.json over it, and commits the result
// to <writable path>/json/app.json. Lookups follow EnvConfigData (current environment, then "common").
//
// Writers must commit to persist:  AppConfigData::getInstance()->setStoryId(id).commit();
// Generic values: getString/putString/getDouble/putDouble (inherited). SoundEngine and SceneManager store their
// floats here through AppDelegate's IO lambdas (getDouble / putDouble + commit).
//
// "__STORY_ID__" in URL paths is replaced with the story id. Keys (JSON names in quotes):
class AppConfigData : public EnvConfigData {
public:
    static const std::string JSON_PATH;                  // "/json/app.json"
    static const std::string JSON_PATH_APP_BUILD_TYPE;   // "/json/product/app.json"
    static const std::string JSON_PATH_APP_PLATFORM;     // "/json/product/android/app.json"

    static const std::string KEY_SITE_URL;                             // "site_url"
    static const std::string KEY_SITE_CONTACT_URL_PATH;                // "site_contact_url_path"
    static const std::string KEY_RESOURCE_URL;                         // "resource_url"
    static const std::string KEY_STORY_JSON_URL_PATH;                  // "story_json_url_path"
    static const std::string KEY_OFFSET_JSON_URL_PATH;                 // "offset_json_url_path"
    static const std::string KEY_RESOURCE_LIST_JSON_URL_PATH;          // "resource_list_json_url_path"
    static const std::string KEY_MAIN_STORY_LIST_JSON_URL_PATH;        // "main_story_list_json_url_path"
    static const std::string KEY_PREVIEW_JSON_URL_PATH;                // "preview_json_url_path"
    static const std::string KEY_PREVIEW_STORY_JSON_URL_PATH;          // "preview_story_json_url_path"
    static const std::string KEY_PREVIEW_RESOURCE_LIST_JSON_URL_PATH;  // "preview_resource_list_json_url_path"
    static const std::string KEY_DEBUG_PAGE_URL_PATH;                  // "debug_page_url_path"
    static const std::string KEY_ERROR_PAGE_URL_PATH;                  // "error_page_url_path"
    static const std::string KEY_PURCHASE_POST_URL_PATH;               // "purchase_post_url_path"
    static const std::string KEY_SUBSCRIBE_POST_URL_PATH;              // "subscribe_post_url_path"
    static const std::string KEY_STORY_ID;                             // "story_id"
    static const std::string KEY_ZIP_FILE_PASSWORD;                    // "zip_file_password"
    static const std::string KEY_FIRST_STORY_ID;                       // "first_story_id"

    static AppConfigData* getInstance();

    virtual ~AppConfigData();
    virtual bool init();

    std::string getSiteUrl();          // site_url ("" when unset)
    std::string getResourceUrl();      // resource_url
    std::string getStoryId();          // story_id
    AppConfigData& setStoryId(std::string storyId);  // not committed
    std::string getZipFilePassword();  // zip_file_password
    std::string getFirstStoryId();     // first_story_id

    std::string getSiteContactUrl();    // site_url + site_contact_url_path (default "/#/top/support/")
    std::string getSiteDebugPageUrl();  // site_url + debug_page_url_path   (default "/#/debug/")
    std::string getSiteErrorPageUrl();  // site_url + error_page_url_path   (default "/#/error/")
    std::string getPurchasePostUrl();   // site_url + purchase_post_url_path
    std::string getSubscribePostUrl();  // site_url + subscribe_post_url_path

    std::string getOffsetJsonUrl();                          // resource_url + offset_json_url_path
    std::string getStoryJsonUrl();                           // ...for the current story id
    std::string getStoryJsonUrl(const char* storyId);        // resource_url + story_json_url_path
    std::string getResourceListJsonUrl();                    // ...for the current story id
    std::string getResourceListJsonUrl(const char* storyId); // resource_url + resource_list_json_url_path
    std::string getMainStoryListJsonUrl();                   // resource_url + main_story_list_json_url_path
    std::string getPreviewJsonUrl();                         // resource_url + preview_json_url_path
    std::string getPreviewStoryJsonUrl();                    // resource_url + preview_story_json_url_path
    std::string getPreviewResourceListJsonUrl();             // resource_url + preview_resource_list_json_url_path

    // Downloaded data, resolved with ResourceManager::getCachePath (device cache dir, else Application/cache/):
    std::string getOffsetJsonFilePath();        // json/offset.json
    std::string getStoryJsonFilePath();         // json/<story id>.json
    std::string getResourceListJsonFilePath();  // json/<story id>_resource.json

    // JSON array of environment names, e.g. ["default"].
    std::string getEnvironmentListJson();

    // `source` with the first `token` replaced by `replacement` (unchanged when `token` is absent).
    std::string replaceString(const std::string& source, const std::string& token, const std::string& replacement);

protected:
    AppConfigData();
    virtual void initDefaultValue();

private:
    std::string withStoryId(const std::string& path, const std::string& storyId);
};

#endif
