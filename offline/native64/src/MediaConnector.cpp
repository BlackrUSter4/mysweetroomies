#include "MediaConnector.h"

#include <jni.h>

#include "platform/android/jni/JniHelper.h"

using cocos2d::JniHelper;
using cocos2d::JniMethodInfo;

namespace {

const char* CLASS_NAME_AUDIO_PROCESSOR = "jp/mfapps/framework/maidengine/util/AudioProcessor";
const char* CLASS_NAME_IMAGE_PROCESSOR = "jp/mfapps/framework/maidengine/util/ImageProcessor";

// Shared body of the twelve "()Z" audio controls.
bool callAudioBool(const char* method) {
    JniMethodInfo info;
    if (!JniHelper::getStaticMethodInfo(info, CLASS_NAME_AUDIO_PROCESSOR, method, "()Z")) {
        return false;
    }
    jboolean result = info.env->CallStaticBooleanMethod(info.classID, info.methodID);
    info.env->DeleteLocalRef(info.classID);
    return result == JNI_TRUE;
}

bool callAudioPlay(const char* method, const char* path) {
    JniMethodInfo info;
    if (!JniHelper::getStaticMethodInfo(info, CLASS_NAME_AUDIO_PROCESSOR, method, "(Ljava/lang/String;)Z")) {
        return false;
    }
    jstring jpath = info.env->NewStringUTF(path);
    jboolean result = info.env->CallStaticBooleanMethod(info.classID, info.methodID, jpath);
    info.env->DeleteLocalRef(info.classID);
    info.env->DeleteLocalRef(jpath);
    return result == JNI_TRUE;
}

float callAudioGetVolume(const char* method) {
    JniMethodInfo info;
    if (!JniHelper::getStaticMethodInfo(info, CLASS_NAME_AUDIO_PROCESSOR, method, "()F")) {
        return 0.0f;
    }
    jfloat volume = info.env->CallStaticFloatMethod(info.classID, info.methodID);
    info.env->DeleteLocalRef(info.classID);
    return volume;
}

void callAudioSetVolume(const char* method, float volume) {
    JniMethodInfo info;
    if (!JniHelper::getStaticMethodInfo(info, CLASS_NAME_AUDIO_PROCESSOR, method, "(F)V")) {
        return;
    }
    info.env->CallStaticVoidMethod(info.classID, info.methodID, static_cast<jdouble>(volume));
    info.env->DeleteLocalRef(info.classID);
}

// The original invokes these "(...)Z" methods with CallStaticFloatMethod and converts the float to an int, which
// reads a boolean's raw bits as a denormal and so reports false even on success. This port uses the call that
// matches the Java signature, so success is reported as true (FilterData::generateFilteredImage sees it).
bool callImageFilter(const char* method, const char* src, const char* dst) {
    JniMethodInfo info;
    if (!JniHelper::getStaticMethodInfo(info, CLASS_NAME_IMAGE_PROCESSOR, method,
                                        "(Ljava/lang/String;Ljava/lang/String;)Z")) {
        return false;
    }
    jstring jsrc = info.env->NewStringUTF(src);
    jstring jdst = info.env->NewStringUTF(dst);
    jboolean result = info.env->CallStaticBooleanMethod(info.classID, info.methodID, jsrc, jdst);
    info.env->DeleteLocalRef(info.classID);
    info.env->DeleteLocalRef(jsrc);
    info.env->DeleteLocalRef(jdst);
    return result == JNI_TRUE;
}

}  // namespace

int MediaConnector::m_seId = 0;
int MediaConnector::m_voiceId = 0;

// orig 001cb57c
float MediaConnector::getAudioDurationTimeSecond(const char* path) {
    JniMethodInfo info;
    if (!JniHelper::getStaticMethodInfo(info, CLASS_NAME_AUDIO_PROCESSOR, "getAudioDurationTime",
                                        "(Ljava/lang/String;)F")) {
        return 0.0f;
    }
    jstring jpath = info.env->NewStringUTF(path);
    jfloat milliseconds = info.env->CallStaticFloatMethod(info.classID, info.methodID, jpath);
    info.env->DeleteLocalRef(info.classID);
    info.env->DeleteLocalRef(jpath);
    return milliseconds / 1000.0f;
}

bool MediaConnector::playBgm(const char* path) { return callAudioPlay("playBgm", path); }      // orig 001cb5e4
bool MediaConnector::playSe(const char* path) { return callAudioPlay("playSe", path); }        // orig 001cb644
bool MediaConnector::playVoice(const char* path) { return callAudioPlay("playVoice", path); }  // orig 001cb6a4

bool MediaConnector::stop() { return callAudioBool("stop"); }                // orig 001cb704
bool MediaConnector::stopBgm() { return callAudioBool("stopBgm"); }          // orig 001cb750
bool MediaConnector::stopSe() { return callAudioBool("stopSe"); }            // orig 001cb79c
bool MediaConnector::stopVoice() { return callAudioBool("stopVoice"); }      // orig 001cb7e8
bool MediaConnector::pause() { return callAudioBool("pause"); }              // orig 001cb834
bool MediaConnector::pauseBgm() { return callAudioBool("pauseBgm"); }        // orig 001cb880
bool MediaConnector::pauseSe() { return callAudioBool("pauseSe"); }          // orig 001cb8cc
bool MediaConnector::pauseVoice() { return callAudioBool("pauseVoice"); }    // orig 001cb918
bool MediaConnector::resume() { return callAudioBool("resume"); }            // orig 001cb964
bool MediaConnector::resumeBgm() { return callAudioBool("resumeBgm"); }      // orig 001cb9b0
bool MediaConnector::resumeSe() { return callAudioBool("resumeSe"); }        // orig 001cb9fc
bool MediaConnector::resumeVoice() { return callAudioBool("resumeVoice"); }  // orig 001cba48

// orig 001cba94
bool MediaConnector::end() {
    stop();
    return true;
}

float MediaConnector::getVolumeBgm() { return callAudioGetVolume("getVolumeBgm"); }      // orig 001cbaa0
float MediaConnector::getVolumeSe() { return callAudioGetVolume("getVolumeSe"); }        // orig 001cbaec
float MediaConnector::getVolumeVoice() { return callAudioGetVolume("getVolumeVoice"); }  // orig 001cbb38

void MediaConnector::setVolumeBgm(float volume) { callAudioSetVolume("setVolumeBgm", volume); }      // orig 001cbb84
void MediaConnector::setVolumeSe(float volume) { callAudioSetVolume("setVolumeSe", volume); }        // orig 001cbbdc
void MediaConnector::setVolumeVoice(float volume) { callAudioSetVolume("setVolumeVoice", volume); }  // orig 001cbc34

// orig 001cbc8c
bool MediaConnector::blur(const char* src, const char* dst, float radius) {
    JniMethodInfo info;
    if (!JniHelper::getStaticMethodInfo(info, CLASS_NAME_IMAGE_PROCESSOR, "blur",
                                        "(Ljava/lang/String;Ljava/lang/String;F)Z")) {
        return false;
    }
    jstring jsrc = info.env->NewStringUTF(src);
    jstring jdst = info.env->NewStringUTF(dst);
    jboolean result =
        info.env->CallStaticBooleanMethod(info.classID, info.methodID, jsrc, jdst, static_cast<jdouble>(radius));
    info.env->DeleteLocalRef(info.classID);
    info.env->DeleteLocalRef(jsrc);
    info.env->DeleteLocalRef(jdst);
    return result == JNI_TRUE;
}

bool MediaConnector::sepia(const char* src, const char* dst) { return callImageFilter("sepia", src, dst); }  // 001cbd18
bool MediaConnector::grayscale(const char* src, const char* dst) {  // orig 001cbd90
    return callImageFilter("grayscale", src, dst);
}
bool MediaConnector::invert(const char* src, const char* dst) { return callImageFilter("invert", src, dst); }  // 001cbe08
