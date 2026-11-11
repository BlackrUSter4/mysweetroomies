#include "LogMaker.h"

#include <cstdarg>
#include <cstdio>
#include <vector>

namespace {

std::string formatVa(const char* format, va_list args) {
    va_list measure;
    va_copy(measure, args);
    int length = vsnprintf(nullptr, 0, format, measure);
    va_end(measure);
    if (length <= 0) {
        return std::string();
    }
    std::vector<char> buffer(static_cast<size_t>(length) + 1);
    vsnprintf(buffer.data(), buffer.size(), format, args);
    return std::string(buffer.data(), static_cast<size_t>(length));
}

}  // namespace

// orig 001d9ff8
std::string LogMaker::CreateOutputString(LOG level, const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::string message = formatVa(format, args);
    va_end(args);

    std::string levelName = (level == LOGTYPE_ERROR) ? "error" : "debug";
    std::string line = "[lv]:" + levelName + "\t[tag]:" + (tag ? tag : "") + "\t[message]:" + message;
    return line;
}

// orig 001d9f10
std::string LogMaker::CreateOutputStringWithLineNumber(const char* file, const char* function, int line,
                                                       const char* format, ...) {
    (void)function;
    va_list args;
    va_start(args, format);
    std::string message = formatVa(format, args);
    va_end(args);

    std::string path(file ? file : "");
    std::string baseName = path.substr(path.rfind('/') + 1);  // npos + 1 == 0 keeps the whole name

    char location[256];
    snprintf(location, sizeof(location), " ... [%s:%d]", baseName.c_str(), line);
    return message + location;
}
