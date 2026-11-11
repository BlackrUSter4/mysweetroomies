#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <string>

// File-system helpers and the game's two-tier lookup for "document" (config JSON) and "cache" (downloaded story)
// files. Paths handed in are relative ("json/app.json"); each kind has a dynamic copy on the device that wins over
// the copy bundled in the APK:
//
//   document: dynamic  <writable path>/<path>              e.g. /data/data/<pkg>/files/json/app.json
//             static   Application/document/<path>          (APK assets)
//   cache:    dynamic  <device cache dir>/<path>            e.g. /data/data/<pkg>/cache/json/offset.json
//             static   Application/cache/<path>             (APK assets)
//
// Every built path goes through removeDuplicateSlash(). Use through getInstance().
class ResourceManager {
public:
    static ResourceManager* getInstance();
    bool init();

    bool isExistFile(const std::string& path);  // CCFileUtils::isFileExist (works for APK assets too)
    bool isExistDir(const std::string& path);   // stat() + S_ISDIR
    // Text before the last '/' (a trailing '/' is ignored): "a/b/c.json" -> "a/b"; "" when there is none.
    std::string getParentDir(const std::string& path);
    std::string removeDuplicateSlash(const std::string& path);  // "a//b///c" -> "a/b/c"
    bool removeFile(const std::string& path);   // true when the file existed and remove() succeeded
    bool mkdirs(const std::string& path);       // mkdir -p with mode 0700; result of the last mkdir()
    bool rmdirs(const std::string& path);       // rm -r; false when the directory cannot be opened/removed

    // Whole file as a string. Absolute paths ("/...") are read with std::ifstream and throw maid::IOException
    // ("cannot load file: <path>") when they cannot be opened; anything else goes through
    // CCFileUtils::getFileData (APK assets / search paths) and yields "" when missing.
    std::string slurp(const std::string& path);
    // Writes `content` to `path` (creating parent directories); false when the file cannot be opened.
    bool spit(const std::string& path, const std::string& content);

    std::string getDynamicCachePath(const std::string& path);
    std::string getStaticCachePath(const std::string& path);
    std::string getCachePath(const std::string& path);         // dynamic if it exists, else static
    std::string getDynamicDocumentPath(const std::string& path);
    std::string getStaticDocumentPath(const std::string& path);
    std::string getDocumentPath(const std::string& path);      // dynamic if it exists, else static

    // NOTE: the original has these two crossed over and so does this port: hasDynamicDocument() checks the
    // dynamic *cache* path and hasDynamicCache() checks the dynamic *document* path.
    bool hasDynamicDocument(const std::string& path);
    bool hasDynamicCache(const std::string& path);

private:
    std::string _readStringData(const std::string& path);
};

#endif
