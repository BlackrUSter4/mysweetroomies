#include "DeviceInfo.h"

#include "cocos2d.h"

namespace {
const std::string FILES_DIR = "/files/";
const std::string CACHE_DIR = "/cache/";
}  // namespace

// orig 001dbffc
std::string DeviceInfo::getDeviceCachePath(const char* path) {
    std::string directory = cocos2d::CCFileUtils::sharedFileUtils()->getWritablePath();
    std::string::size_type files = directory.find(FILES_DIR, 0);
    // The original replaces unconditionally; with npos std::string::replace would throw, so only an Android
    // writable path (which always contains "/files/") takes the swap.
    if (files != std::string::npos) {
        directory.replace(files, FILES_DIR.length(), CACHE_DIR);
    }
    directory += path;
    return directory;
}
