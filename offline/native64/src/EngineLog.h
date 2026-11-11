#ifndef MAID_ENGINELOG_H
#define MAID_ENGINELOG_H

#include <cstdarg>
#include <string>

namespace maid {

// Severity for EngineLog::_Log. Values map to Android priorities VERBOSE..ERROR (value + 1).
// GUESS: enumerator names are invented; the values 1..5 come from the original's switch.
enum LogLevel {
    LOG_LEVEL_VERBOSE = 1,
    LOG_LEVEL_DEBUG = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_WARN = 4,
    LOG_LEVEL_ERROR = 5,
};

class EngineLog {
public:
    // Writes "<message> --- <file stem>.<function> (<line>)" to logcat under the tag "EngineLog".
    // Nothing is written unless bit 0x200 of `flags` is set, and LOG_LEVEL_VERBOSE is always dropped.
    // `file` may be a full path: the directory and the extension are stripped.
    // GUESS: the meaning of `flags` (an int whose bit 9 gates output) is unknown; no caller exists in the game.
    static void _Log(const LogLevel& level, const std::string& file, int line, const std::string& function, int flags,
                     const std::string& format, va_list args);
};

}  // namespace maid

#endif
