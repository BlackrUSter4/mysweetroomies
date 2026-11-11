// JNI entry points the game defines itself (orig re/c/JNI.c). The stock cocos2d-x ones (nativeRender, touches,
// Cocos2dxHelper, Cocos2dxBitmap, ...) come from the engine build.
//
//   JNI_OnLoad                                                    stores the VM for JniHelper
//   org.cocos2dx.lib.Cocos2dxRenderer.nativeInit(II)V             cocos2d-x 2.x template: start or restore GL
//   jp.mfapps.common.jni.SceneConnector.jni_startScene(String,String)V        static
//   jp.mfapps.common.jni.AppConfigConnector.jni_*                 instance methods, all on AppConfigData

#include <android/log.h>
#include <jni.h>

#include <string>

#include "AppConfigData.h"
#include "AppDelegate.h"
#include "SceneConnector.h"
#include "cocos2d.h"
#include "platform/android/jni/JniHelper.h"

using namespace cocos2d;

namespace {

// UTF-8 copy of a Java string; null maps to "" (the original would abort inside GetStringUTFChars).
std::string toStdString(JNIEnv* env, jstring value) {
    if (value == nullptr) {
        return std::string();
    }
    const char* chars = env->GetStringUTFChars(value, nullptr);
    std::string result(chars ? chars : "");
    env->ReleaseStringUTFChars(value, chars);
    return result;
}

jstring toJavaString(JNIEnv* env, const std::string& value) {
    return env->NewStringUTF(value.c_str());
}

AppConfigData* config() {
    return AppConfigData::getInstance();
}

}  // namespace

extern "C" {

// orig 001c06ec
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    (void)reserved;
    JniHelper::setJavaVM(vm);
    return JNI_VERSION_1_4;
}

// orig 001c06fc
JNIEXPORT void JNICALL Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInit(JNIEnv* env, jclass clazz, jint width,
                                                                         jint height) {
    (void)env;
    (void)clazz;
    if (CCDirector::sharedDirector()->getOpenGLView() == nullptr) {
        CCEGLView* view = CCEGLView::sharedOpenGLView();
        view->setFrameSize(static_cast<float>(width), static_cast<float>(height));
        // Owned by CCApplication's singleton for the life of the process, as in the cocos2d-x template.
        new AppDelegate();
        CCApplication::sharedApplication()->run();
    } else {
        // The GL context was recreated (app resumed): rebuild every GL object.
        ccGLInvalidateStateCache();
        CCShaderCache::sharedShaderCache()->reloadDefaultShaders();
        ccDrawInit();
        CCTextureCache::reloadAllTextures();
        CCNotificationCenter::sharedNotificationCenter()->postNotification(EVENT_COME_TO_FOREGROUND, nullptr);
        CCDirector::sharedDirector()->setGLDefaultValues();
    }
}

// orig 001c0830
JNIEXPORT void JNICALL Java_jp_mfapps_common_jni_SceneConnector_jni_1startScene(JNIEnv* env, jclass clazz,
                                                                                jstring sceneName,
                                                                                jstring callbackParameter) {
    (void)clazz;
    __android_log_print(ANDROID_LOG_DEBUG, "SceneConnector", "[scene_connector_wrapper] success to call startScene");
    std::string name = toStdString(env, sceneName);
    std::string parameter = toStdString(env, callbackParameter);
    SceneConnector::getInstance()->startScene(name.c_str(), parameter.c_str());
}

// --- AppConfigConnector (orig 001c0a5c .. 001c0ff4) ---

JNIEXPORT jstring JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getStoryJsonFilePath(JNIEnv* env,
                                                                                                jobject thiz) {
    (void)thiz;
    return toJavaString(env, config()->getStoryJsonFilePath());
}

JNIEXPORT jstring JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getOffsetJsonFilePath(JNIEnv* env,
                                                                                                 jobject thiz) {
    (void)thiz;
    return toJavaString(env, config()->getOffsetJsonFilePath());
}

JNIEXPORT jstring JNICALL
Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getResourceListJsonFilePath(JNIEnv* env, jobject thiz) {
    (void)thiz;
    return toJavaString(env, config()->getResourceListJsonFilePath());
}

JNIEXPORT jstring JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getSiteUrl(JNIEnv* env, jobject thiz) {
    (void)thiz;
    return toJavaString(env, config()->getSiteUrl());
}

JNIEXPORT jstring JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getResourceUrl(JNIEnv* env,
                                                                                          jobject thiz) {
    (void)thiz;
    return toJavaString(env, config()->getResourceUrl());
}

JNIEXPORT jstring JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getStoryJsonUrl__(JNIEnv* env,
                                                                                             jobject thiz) {
    (void)thiz;
    return toJavaString(env, config()->getStoryJsonUrl());
}

JNIEXPORT jstring JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getStoryJsonUrl__Ljava_lang_String_2(
    JNIEnv* env, jobject thiz, jstring storyId) {
    (void)thiz;
    std::string id = toStdString(env, storyId);
    return toJavaString(env, config()->getStoryJsonUrl(id.c_str()));
}

JNIEXPORT jstring JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getOffsetJsonUrl(JNIEnv* env,
                                                                                            jobject thiz) {
    (void)thiz;
    return toJavaString(env, config()->getOffsetJsonUrl());
}

JNIEXPORT jstring JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getResourceListJsonUrl__(JNIEnv* env,
                                                                                                    jobject thiz) {
    (void)thiz;
    return toJavaString(env, config()->getResourceListJsonUrl());
}

JNIEXPORT jstring JNICALL
Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getResourceListJsonUrl__Ljava_lang_String_2(JNIEnv* env,
                                                                                               jobject thiz,
                                                                                               jstring storyId) {
    (void)thiz;
    std::string id = toStdString(env, storyId);
    return toJavaString(env, config()->getResourceListJsonUrl(id.c_str()));
}

JNIEXPORT jstring JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getMainStoryListJsonUrl(JNIEnv* env,
                                                                                                   jobject thiz) {
    (void)thiz;
    return toJavaString(env, config()->getMainStoryListJsonUrl());
}

JNIEXPORT jstring JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getPreviewJsonUrl(JNIEnv* env,
                                                                                             jobject thiz) {
    (void)thiz;
    return toJavaString(env, config()->getPreviewJsonUrl());
}

JNIEXPORT jstring JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getPreviewStoryJsonUrl(JNIEnv* env,
                                                                                                  jobject thiz) {
    (void)thiz;
    return toJavaString(env, config()->getPreviewStoryJsonUrl());
}

JNIEXPORT jstring JNICALL
Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getPreviewResourceListJsonUrl(JNIEnv* env, jobject thiz) {
    (void)thiz;
    return toJavaString(env, config()->getPreviewResourceListJsonUrl());
}

JNIEXPORT jstring JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getPurchasePostUrl(JNIEnv* env,
                                                                                              jobject thiz) {
    (void)thiz;
    return toJavaString(env, config()->getPurchasePostUrl());
}

JNIEXPORT jstring JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getSubscribePostUrl(JNIEnv* env,
                                                                                               jobject thiz) {
    (void)thiz;
    return toJavaString(env, config()->getSubscribePostUrl());
}

JNIEXPORT jstring JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getStoryId(JNIEnv* env, jobject thiz) {
    (void)thiz;
    return toJavaString(env, config()->getStoryId());
}

JNIEXPORT void JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1setStoryId(JNIEnv* env, jobject thiz,
                                                                                   jstring storyId) {
    (void)thiz;
    config()->setStoryId(toStdString(env, storyId)).commit();
}

JNIEXPORT jstring JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getZipFilePassword(JNIEnv* env,
                                                                                              jobject thiz) {
    (void)thiz;
    return toJavaString(env, config()->getZipFilePassword());
}

JNIEXPORT jstring JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getFirstStoryId(JNIEnv* env,
                                                                                           jobject thiz) {
    (void)thiz;
    return toJavaString(env, config()->getFirstStoryId());
}

JNIEXPORT jstring JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getEnvironmentListJson(JNIEnv* env,
                                                                                                  jobject thiz) {
    (void)thiz;
    return toJavaString(env, config()->getEnvironmentListJson());
}

JNIEXPORT jstring JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getString(JNIEnv* env, jobject thiz,
                                                                                     jstring key,
                                                                                     jstring defaultValue) {
    (void)thiz;
    std::string keyText = toStdString(env, key);
    std::string defaultText = toStdString(env, defaultValue);
    return toJavaString(env, config()->getString(keyText, defaultText));
}

JNIEXPORT void JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1putString(JNIEnv* env, jobject thiz,
                                                                                  jstring key, jstring value) {
    (void)thiz;
    std::string keyText = toStdString(env, key);
    std::string valueText = toStdString(env, value);
    config()->putString(keyText, valueText).commit();
}

JNIEXPORT jstring JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1getEnv(JNIEnv* env, jobject thiz) {
    (void)thiz;
    return toJavaString(env, config()->getEnv());
}

JNIEXPORT void JNICALL Java_jp_mfapps_common_jni_AppConfigConnector_jni_1setEnv(JNIEnv* env, jobject thiz,
                                                                               jstring environment) {
    (void)thiz;
    config()->switching(toStdString(env, environment)).commit();
}

}  // extern "C"
