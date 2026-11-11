#ifndef MEDIACONNECTOR_H
#define MEDIACONNECTOR_H

// Thin JNI bridge to the Java audio and image helpers. Every call is a static Java method; when the method cannot be
// resolved the call is skipped and false / 0 is returned.
//
// Audio: jp/mfapps/framework/maidengine/util/AudioProcessor (BGM, SE and voice channels; paths are asset paths
// such as "Application/sound/bgm/splash/13_01.ogg"). Volumes are 0..1.
// Images: jp/mfapps/framework/maidengine/util/ImageProcessor writes a filtered copy of `src` to `dst`.
class MediaConnector {
public:
    static int m_seId;     // unused by the game; kept from the original
    static int m_voiceId;  // unused by the game; kept from the original

    static float getAudioDurationTimeSecond(const char* path);  // AudioProcessor reports ms; converted to seconds

    static bool playBgm(const char* path);
    static bool playSe(const char* path);
    static bool playVoice(const char* path);

    static bool stop();  // all channels
    static bool stopBgm();
    static bool stopSe();
    static bool stopVoice();
    static bool pause();
    static bool pauseBgm();
    static bool pauseSe();
    static bool pauseVoice();
    static bool resume();
    static bool resumeBgm();
    static bool resumeSe();
    static bool resumeVoice();
    static bool end();  // stop(); always true

    static float getVolumeBgm();
    static float getVolumeSe();
    static float getVolumeVoice();
    static void setVolumeBgm(float volume);
    static void setVolumeSe(float volume);
    static void setVolumeVoice(float volume);

    static bool blur(const char* src, const char* dst, float radius);
    static bool sepia(const char* src, const char* dst);
    static bool grayscale(const char* src, const char* dst);
    static bool invert(const char* src, const char* dst);
};

#endif
