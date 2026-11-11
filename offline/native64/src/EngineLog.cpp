#include "EngineLog.h"

#include <android/log.h>
#include <cstdio>

namespace maid {

// orig 001cbf2c
void EngineLog::_Log(const LogLevel& level, const std::string& file, int line, const std::string& function, int flags,
                     const std::string& format, va_list args) {
    if (!(flags & 0x200) || level < LOG_LEVEL_DEBUG) {
        return;
    }

    char message[0x800];
    if (vsnprintf(message, sizeof(message), format.c_str(), args) < 0) {
        return;
    }

    std::string stem = file;
    std::string::size_type slash = stem.rfind("/");
    if (slash != std::string::npos) {
        stem = stem.substr(slash + 1);
    }
    std::string::size_type dot = stem.rfind(".");
    if (dot != std::string::npos) {
        stem = stem.substr(0, dot);
    }

    char output[0x800];
    if (snprintf(output, sizeof(output), "%s --- %s.%s (%d)", message, stem.c_str(), function.c_str(), line) < 0) {
        return;
    }

    int priority;
    switch (level) {
        case LOG_LEVEL_VERBOSE: priority = ANDROID_LOG_VERBOSE; break;
        case LOG_LEVEL_DEBUG:   priority = ANDROID_LOG_DEBUG; break;
        case LOG_LEVEL_INFO:    priority = ANDROID_LOG_INFO; break;
        case LOG_LEVEL_WARN:    priority = ANDROID_LOG_WARN; break;
        case LOG_LEVEL_ERROR:   priority = ANDROID_LOG_ERROR; break;
        default: return;
    }
    __android_log_write(priority, "EngineLog", output);
}

}  // namespace maid
