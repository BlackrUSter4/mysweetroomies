#include "ResourceManager.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

#include "DeviceInfo.h"
#include "IOException.h"
#include "cocos2d.h"

using cocos2d::CCFileUtils;

// orig 001c6120
ResourceManager* ResourceManager::getInstance() {
    static ResourceManager instance;
    static bool initialized = instance.init();
    (void)initialized;
    return &instance;
}

// orig 001cd214
bool ResourceManager::init() {
    return true;
}

// orig 001cd216
bool ResourceManager::isExistFile(const std::string& path) {
    return CCFileUtils::sharedFileUtils()->isFileExist(path);
}

// orig 001cd228
bool ResourceManager::isExistDir(const std::string& path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
}

// orig 001cd254
std::string ResourceManager::getParentDir(const std::string& path) {
    for (int i = static_cast<int>(path.length()) - 2; i >= 0; --i) {
        if (path[static_cast<size_t>(i)] == '/') {
            return path.substr(0, static_cast<size_t>(i));
        }
    }
    return "";
}

// orig 001cd290
std::string ResourceManager::removeDuplicateSlash(const std::string& path) {
    std::string result;
    result.reserve(path.length());
    for (size_t i = 0; i < path.length(); ++i) {
        if (i == 0 || path[i] != '/' || path[i - 1] != '/') {
            result += path[i];
        }
    }
    return result;
}

// orig 001cd2e4
bool ResourceManager::removeFile(const std::string& path) {
    if (!isExistFile(path)) {
        return false;
    }
    return remove(path.c_str()) == 0;
}

// orig 001cd2fc
bool ResourceManager::mkdirs(const std::string& path) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s", path.c_str());
    size_t length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '/') {
        buffer[length - 1] = '\0';
    }
    // Start at index 1 so a leading '/' is not turned into mkdir("").
    for (char* p = buffer + 1; *p != '\0'; ++p) {
        if (*p == '/') {
            *p = '\0';
            mkdir(buffer, 0700);
            *p = '/';
        }
    }
    return mkdir(buffer, 0700) == 0;
}

// orig 001cd378
bool ResourceManager::spit(const std::string& path, const std::string& content) {
    std::string parent = getInstance()->getParentDir(path);
    if (parent != "" && !getInstance()->isExistDir(parent)) {
        getInstance()->mkdirs(parent);
    }
    std::ofstream out(path.c_str(), std::ios::out);
    bool ok = out.good();
    if (ok) {
        out << content;
        out.close();
    }
    return ok;
}

// orig 001cd408
bool ResourceManager::rmdirs(const std::string& path) {
    DIR* directory = opendir(path.c_str());
    if (directory == nullptr) {
        return false;
    }
    int result = 0;
    while (result == 0) {
        struct dirent* entry = readdir(directory);
        if (entry == nullptr) {
            break;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        std::string child = path + "/" + entry->d_name;
        struct stat info;
        if (stat(child.c_str(), &info) != 0) {
            result = -1;
        } else if (S_ISDIR(info.st_mode)) {
            // The original feeds rmdirs()' bool back into an int status, so a successful subtree is 1 and stops
            // the scan exactly like an error would; kept as is.
            result = rmdirs(child) ? 1 : 0;
        } else {
            result = unlink(child.c_str());
        }
    }
    closedir(directory);
    if (result == 0) {
        result = rmdir(path.c_str());
    }
    return result != -1;
}

// orig 001cd594
std::string ResourceManager::getDynamicCachePath(const std::string& path) {
    return removeDuplicateSlash(DeviceInfo::getDeviceCachePath("") + path);
}

// orig 001cd5e4
bool ResourceManager::hasDynamicDocument(const std::string& path) {
    return isExistFile(getDynamicCachePath(path));
}

// orig 001cd612
std::string ResourceManager::getDynamicDocumentPath(const std::string& path) {
    return removeDuplicateSlash(CCFileUtils::sharedFileUtils()->getWritablePath() + path);
}

// orig 001cd662
bool ResourceManager::hasDynamicCache(const std::string& path) {
    return isExistFile(getDynamicDocumentPath(path));
}

// orig 001cd690
std::string ResourceManager::getStaticCachePath(const std::string& path) {
    return removeDuplicateSlash(std::string("Application/cache/") + path);
}

// orig 001cd6e4
std::string ResourceManager::getCachePath(const std::string& path) {
    std::string dynamicPath = getDynamicCachePath(path);
    if (isExistFile(dynamicPath)) {
        return dynamicPath;
    }
    return getStaticCachePath(path);
}

// orig 001cd72c
std::string ResourceManager::getStaticDocumentPath(const std::string& path) {
    return removeDuplicateSlash(std::string("Application/document/") + path);
}

// orig 001cd780
std::string ResourceManager::getDocumentPath(const std::string& path) {
    std::string dynamicPath = getDynamicDocumentPath(path);
    if (isExistFile(dynamicPath)) {
        return dynamicPath;
    }
    return getStaticDocumentPath(path);
}

// orig 001cd858
std::string ResourceManager::_readStringData(const std::string& path) {
    std::ifstream in(path.c_str(), std::ios::in);
    if (!in.good()) {
        throw maid::IOException("cannot load file: " + path);
    }
    std::stringstream content;
    content << in.rdbuf();
    return content.str();
}

// orig 001cd904
std::string ResourceManager::slurp(const std::string& path) {
    if (path.compare(0, 1, "/") == 0) {
        return _readStringData(path);
    }
    unsigned long size = 0;
    unsigned char* data = CCFileUtils::sharedFileUtils()->getFileData(path.c_str(), "r", &size);
    std::string content;
    if (data != nullptr) {
        // The original copies into a NUL-terminated buffer and assigns it as a C string, so text stops at the
        // first NUL byte.
        const char* text = reinterpret_cast<const char*>(data);
        content.assign(text, strnlen(text, static_cast<size_t>(size)));
        delete[] data;
    }
    return content;
}
