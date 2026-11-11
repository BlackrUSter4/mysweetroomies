#ifndef LOGMAKER_H
#define LOGMAKER_H

#include <string>

// Log level passed to LogMaker::CreateOutputString() and NativeConnector::Log().
// GUESS: enumerator names are invented; only the values 1 ("debug") and 2 ("error") appear in the original.
enum LOG {
    LOGTYPE_NONE = 0,
    LOGTYPE_DEBUG = 1,
    LOGTYPE_ERROR = 2,
};

// Builds log lines; it does not print anything itself.
class LogMaker {
public:
    // "[lv]:<debug|error>\t[tag]:<tag>\t[message]:<printf(format, ...)>"  (any level other than LOGTYPE_ERROR is
    // written as "debug").
    static std::string CreateOutputString(LOG level, const char* tag, const char* format, ...);

    // "<printf(format, ...)> ... [<basename of file>:<line>]". `function` is accepted but unused, as in the original.
    static std::string CreateOutputStringWithLineNumber(const char* file, const char* function, int line,
                                                        const char* format, ...);
};

#endif
