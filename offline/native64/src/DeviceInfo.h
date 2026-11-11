#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include <string>

class DeviceInfo {
public:
    // The app's cache directory plus `path`: CCFileUtils' writable path with its "/files/" component swapped for
    // "/cache/", e.g. "/data/data/<package>/cache/" + path.
    static std::string getDeviceCachePath(const char* path);
};

#endif
