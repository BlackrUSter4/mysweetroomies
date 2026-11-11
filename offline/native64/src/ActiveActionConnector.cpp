#include "ActiveActionConnector.h"

#include <jni.h>

#include "platform/android/jni/JniHelper.h"

// orig 001c57c0
bool ActiveActionConnector::init() {
    return true;
}

// orig 001c57e4
void ActiveActionConnector::clearCache() {
    cocos2d::JniMethodInfo info;
    // The original calls without checking the lookup; skipping on failure avoids calling through a null method.
    if (!cocos2d::JniHelper::getStaticMethodInfo(info, "jp/mfapps/common/jni/ActiveActionConnector", "jni_clearCache",
                                                 "()V")) {
        return;
    }
    info.env->CallStaticVoidMethod(info.classID, info.methodID);
    info.env->DeleteLocalRef(info.classID);
}
