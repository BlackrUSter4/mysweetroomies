#include "AppTools.h"

#include <cstdarg>
#include <cstdio>
#include <string>

bool AppTools::isAssertTest = false;

// orig 001cbe80
bool AppTools::isTest() {
    return isAssertTest;
}

// orig 001cbf1c
void AppTools::setTest(bool test) {
    isAssertTest = test;
}

// orig 001cbe90
void AppTools::Assert(bool condition, const char* format, ...) {
    // The original formats into a ~500-byte stack buffer and keeps the text in a std::string it never uses; the
    // formatting is kept so side effects of the arguments (none in practice) stay identical.
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    std::string message(buffer);
    (void)message;

    if (!condition && isTest()) {
        throw "AssertTestException";
    }
}
