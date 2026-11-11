#include "AppConfigData.h"

#include "ResourceManager.h"

namespace {
// orig: build-flavour constants of the shipped APK (a "product" build for "android").
const std::string BUILD_TYPE = "product";
const std::string PLATFORM = "android";

const std::string STORY_ID_TOKEN = "__STORY_ID__";
const std::string STORY_JSON_FILE = "json/__STORY_ID__.json";
const std::string OFFSET_JSON_FILE = "json/offset.json";
const std::string RESOURCE_LIST_JSON_FILE = "json/__STORY_ID___resource.json";
const std::string DEFAULT_SITE_CONTACT_URL_PATH = "/#/top/support/";
const std::string DEFAULT_DEBUG_PAGE_URL_PATH = "/#/debug/";
const std::string DEFAULT_ERROR_PAGE_URL_PATH = "/#/error/";
}  // namespace

const std::string AppConfigData::JSON_PATH = "/json/app.json";
const std::string AppConfigData::JSON_PATH_APP_BUILD_TYPE = "/json/" + BUILD_TYPE + "/app.json";
const std::string AppConfigData::JSON_PATH_APP_PLATFORM = "/json/" + BUILD_TYPE + "/" + PLATFORM + "/app.json";

const std::string AppConfigData::KEY_SITE_URL = "site_url";
const std::string AppConfigData::KEY_SITE_CONTACT_URL_PATH = "site_contact_url_path";
const std::string AppConfigData::KEY_RESOURCE_URL = "resource_url";
const std::string AppConfigData::KEY_STORY_JSON_URL_PATH = "story_json_url_path";
const std::string AppConfigData::KEY_OFFSET_JSON_URL_PATH = "offset_json_url_path";
const std::string AppConfigData::KEY_RESOURCE_LIST_JSON_URL_PATH = "resource_list_json_url_path";
const std::string AppConfigData::KEY_MAIN_STORY_LIST_JSON_URL_PATH = "main_story_list_json_url_path";
const std::string AppConfigData::KEY_PREVIEW_JSON_URL_PATH = "preview_json_url_path";
const std::string AppConfigData::KEY_PREVIEW_STORY_JSON_URL_PATH = "preview_story_json_url_path";
const std::string AppConfigData::KEY_PREVIEW_RESOURCE_LIST_JSON_URL_PATH = "preview_resource_list_json_url_path";
const std::string AppConfigData::KEY_DEBUG_PAGE_URL_PATH = "debug_page_url_path";
const std::string AppConfigData::KEY_ERROR_PAGE_URL_PATH = "error_page_url_path";
const std::string AppConfigData::KEY_PURCHASE_POST_URL_PATH = "purchase_post_url_path";
const std::string AppConfigData::KEY_SUBSCRIBE_POST_URL_PATH = "subscribe_post_url_path";
const std::string AppConfigData::KEY_STORY_ID = "story_id";
const std::string AppConfigData::KEY_ZIP_FILE_PASSWORD = "zip_file_password";
const std::string AppConfigData::KEY_FIRST_STORY_ID = "first_story_id";

// orig 001c0990 (the constructor chain and init() are inlined there)
AppConfigData* AppConfigData::getInstance() {
    static AppConfigData instance;
    return &instance;
}

AppConfigData::AppConfigData() {
    init();
}

// orig 001c0904
AppConfigData::~AppConfigData() {}

// orig 001c0932
bool AppConfigData::init() {
    return SwitchableConfigData::init();
}

// orig 001c61c0 — runs from init(): base file, then the build-type and platform overlays, then persist.
void AppConfigData::initDefaultValue() {
    load(JSON_PATH);

    EnvConfigData buildType;
    buildType.load(JSON_PATH_APP_BUILD_TYPE);
    merge(buildType);

    EnvConfigData platform;
    platform.load(JSON_PATH_APP_PLATFORM);
    merge(platform);

    commit();
}

// orig 001c63e4
std::string AppConfigData::replaceString(const std::string& source, const std::string& token,
                                         const std::string& replacement) {
    std::string result = source;
    std::string::size_type position = result.find(token, 0);
    if (position == std::string::npos) {
        return result;
    }
    return result.replace(position, token.length(), replacement);
}

std::string AppConfigData::withStoryId(const std::string& path, const std::string& storyId) {
    return replaceString(path, STORY_ID_TOKEN, storyId);
}

// orig 001c625c
std::string AppConfigData::getSiteUrl() {
    return getString(KEY_SITE_URL, "");
}

// orig 001c62a0
std::string AppConfigData::getResourceUrl() {
    return getString(KEY_RESOURCE_URL, "");
}

// orig 001c62e4
std::string AppConfigData::getStoryId() {
    return getString(KEY_STORY_ID, "");
}

// orig 001c6328
AppConfigData& AppConfigData::setStoryId(std::string storyId) {
    putString(KEY_STORY_ID, storyId);
    return *this;
}

// orig 001c635c
std::string AppConfigData::getZipFilePassword() {
    return getString(KEY_ZIP_FILE_PASSWORD, "");
}

// orig 001c63a0
std::string AppConfigData::getFirstStoryId() {
    return getString(KEY_FIRST_STORY_ID, "");
}

// orig 001c6240
std::string AppConfigData::getOffsetJsonFilePath() {
    return ResourceManager::getInstance()->getCachePath(OFFSET_JSON_FILE);
}

// orig 001c6444
std::string AppConfigData::getStoryJsonFilePath() {
    return ResourceManager::getInstance()->getCachePath(withStoryId(STORY_JSON_FILE, getStoryId()));
}

// orig 001c649c
std::string AppConfigData::getResourceListJsonFilePath() {
    return ResourceManager::getInstance()->getCachePath(withStoryId(RESOURCE_LIST_JSON_FILE, getStoryId()));
}

// orig 001c6518
std::string AppConfigData::getEnvironmentListJson() {
    std::vector<std::string> environments = getInstance()->getEnvList();
    std::string json = "[";
    std::string separator = "";
    for (size_t i = 0; i < environments.size(); ++i) {
        json += separator + "\"" + environments[i] + "\"";
        separator = ",";
    }
    json += "]";
    return json;
}

// orig 001c66b8
std::string AppConfigData::getSiteContactUrl() {
    std::string site = getSiteUrl();
    return site + getString(KEY_SITE_CONTACT_URL_PATH, DEFAULT_SITE_CONTACT_URL_PATH);
}

// orig 001c6790
std::string AppConfigData::getSiteDebugPageUrl() {
    std::string site = getSiteUrl();
    return site + getString(KEY_DEBUG_PAGE_URL_PATH, DEFAULT_DEBUG_PAGE_URL_PATH);
}

// orig 001c68e4
std::string AppConfigData::getSiteErrorPageUrl() {
    std::string site = getSiteUrl();
    return site + getString(KEY_ERROR_PAGE_URL_PATH, DEFAULT_ERROR_PAGE_URL_PATH);
}

// orig 001c67ec
std::string AppConfigData::getPurchasePostUrl() {
    std::string site = getSiteUrl();
    return site + getString(KEY_PURCHASE_POST_URL_PATH, "");
}

// orig 001c6868
std::string AppConfigData::getSubscribePostUrl() {
    std::string site = getSiteUrl();
    return site + getString(KEY_SUBSCRIBE_POST_URL_PATH, "");
}

// orig 001c6714
std::string AppConfigData::getOffsetJsonUrl() {
    std::string resource = getResourceUrl();
    return resource + getString(KEY_OFFSET_JSON_URL_PATH, "");
}

// orig 001c6940
std::string AppConfigData::getStoryJsonUrl(const char* storyId) {
    std::string id(storyId);
    std::string path = withStoryId(getString(KEY_STORY_JSON_URL_PATH, ""), id);
    return getResourceUrl() + path;
}

// orig 001c69fc
std::string AppConfigData::getStoryJsonUrl() {
    std::string id = getStoryId();
    return getStoryJsonUrl(id.c_str());
}

// orig 001c6a28
std::string AppConfigData::getResourceListJsonUrl(const char* storyId) {
    std::string id(storyId);
    std::string path = withStoryId(getString(KEY_RESOURCE_LIST_JSON_URL_PATH, ""), id);
    return getResourceUrl() + path;
}

// orig 001c6ae4
std::string AppConfigData::getResourceListJsonUrl() {
    std::string id = getStoryId();
    return getResourceListJsonUrl(id.c_str());
}

// orig 001c6b10
std::string AppConfigData::getMainStoryListJsonUrl() {
    std::string path = withStoryId(getString(KEY_MAIN_STORY_LIST_JSON_URL_PATH, ""), getStoryId());
    return getResourceUrl() + path;
}

// orig 001c6bc8
std::string AppConfigData::getPreviewJsonUrl() {
    std::string path = withStoryId(getString(KEY_PREVIEW_JSON_URL_PATH, ""), getStoryId());
    return getResourceUrl() + path;
}

// orig 001c6c80
std::string AppConfigData::getPreviewStoryJsonUrl() {
    std::string path = withStoryId(getString(KEY_PREVIEW_STORY_JSON_URL_PATH, ""), getStoryId());
    return getResourceUrl() + path;
}

// orig 001c6d38
std::string AppConfigData::getPreviewResourceListJsonUrl() {
    std::string path = withStoryId(getString(KEY_PREVIEW_RESOURCE_LIST_JSON_URL_PATH, ""), getStoryId());
    return getResourceUrl() + path;
}
