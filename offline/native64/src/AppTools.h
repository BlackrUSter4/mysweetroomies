#ifndef APPTOOLS_H
#define APPTOOLS_H

// Debug helpers shared by the game code.
//
// AppTools::Assert() formats its message (printf-style) but never logs it; the only observable effect is that a failed
// assertion throws `const char*` "AssertTestException" while test mode is on (setTest(true)). With test mode off
// (the default) a failed assertion is silently ignored, exactly like the shipped build.
class AppTools {
public:
    static bool isAssertTest;  // test-mode flag behind isTest()/setTest()

    static bool isTest();
    static void setTest(bool test);

    // Throws `const char*` "AssertTestException" when `condition` is false and test mode is on.
    static void Assert(bool condition, const char* format, ...);
};

#endif
