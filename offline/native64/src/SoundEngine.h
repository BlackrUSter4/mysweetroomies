#ifndef SOUNDENGINE_H
#define SOUNDENGINE_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

struct SceneData;
struct SoundData;

// Music, sound effects and voice for the scenes, played through MediaConnector (Java AudioProcessor).
//
// Lifetime per scene that makes sound (StartGameScene, OptionMenuScene, ConversationScene, AppDelegate at launch):
//   SoundEngine::getInstance()->prepare(load, save);   // onEnter / init
//   ...
//   SoundEngine::getInstance()->release();             // onExit
// prepare() registers the engine with SceneManager as listener "sound_engine" (so story scenes drive it: setup ->
// preload, pre_render -> clear SE/voice, main_render -> play the scene's SoundData, post_effect -> stop SE and voice,
// finish -> stop everything), installs the volume persistence callbacks and restores the saved volumes.
// The callbacks every caller passes store floats in AppConfigData:
//   load = [](const std::string& k, float d) -> const float { return AppConfigData::getInstance()->getDouble(k, d); }
//   save = [](const std::string& k, float v) { AppConfigData::getInstance()->putDouble(k, v).commit(); }
//
// Volumes are 0..1 (clamped). Persisted keys: "sound_engine.volume.bgm" and "sound_engine.volume.se" (default 1).
// "Se" volume is applied to both the SE and the voice channel. "Mute" means both volumes are 0; listeners set with
// setMuteChangeListener() hear about entering/leaving that state.
class SoundEngine {
public:
    typedef std::function<const float(const std::string&, float)> LoadFunction;  // (key, default) -> stored value
    typedef std::function<void(const std::string&, float)> SaveFunction;          // (key, value)

    static SoundEngine* getInstance();
    ~SoundEngine();
    bool init();

    void prepare(LoadFunction load, SaveFunction save);
    // Unregisters from SceneManager, restores the no-op IO, forgets the current paths, stops all audio and ends
    // the Java player.
    void release();

    // Direct playback (no path bookkeeping).
    void playBgm(const char* path);
    void playSe(const char* path);
    void playVoice(const char* path);
    // Replay the stored paths of the current scene (empty paths are skipped). ConversationScene uses
    // playVoice() to repeat the current line.
    void playBgm();
    void playSe();
    void playVoice();
    void play();  // voice, SE, BGM

    // Applies a scene's sound: BGM "__RESET__" stops the music, a different BGM path restarts it, the same path
    // keeps it playing; SE and voice are always (re)played when non-empty.
    void playSoundData(std::shared_ptr<SoundData> sound);
    void preloadSoundData(std::shared_ptr<SoundData> sound);  // no-op, as in the original

    void stop();    // all channels
    void pause();   // all channels (AppDelegate on background)
    void resume();  // all channels (AppDelegate on foreground)

    std::string getBgmPath();
    std::string getSePath();
    std::string getVoicePath();
    void clearSoundPath();        // BGM, SE and voice paths
    void clearSoundPathBgm();
    void clearSoundPathEffect();  // SE and voice paths

    float getVolumeBgm();
    float getVolumeSe();
    void setVolumeBgm(float volume);  // clamps, applies, saves, notifies on mute change
    void setVolumeSe(float volume);   // ... also sets the voice volume
    float getDefaultBgmValue();       // 1
    float getDefaultSeValue();        // 1

    bool isMute();     // both volumes <= 0
    void setMute();    // remembers the current volumes, then sets both to 0
    void resetMute();  // restores a remembered volume for every channel that is at 0
    void setMuteChangeListener(std::function<void(bool)> listener);  // called with isMute()
    void setDefaultMuteChangeListener();                             // no-op listener
    void notifyMuteChange();

    void setIO(LoadFunction load, SaveFunction save);
    void setDefaultIO();  // load returns 1, save does nothing
    void load();          // restore both volumes through the load function
    float loadValue(const std::string& key, float defaultValue);
    void saveValue(const std::string& key, float value);

    // SceneEventListener factories bound to this engine (used by prepare()).
    std::function<void(const std::vector<std::shared_ptr<SceneData>>&)> onSetup();
    std::function<void(std::shared_ptr<SceneData>)> onMainRender();
    std::function<void(std::shared_ptr<SceneData>, std::shared_ptr<SceneData>)> onPreRender();
    std::function<void(std::shared_ptr<SceneData>, std::shared_ptr<SceneData>)> onPreEffect();
    std::function<void(std::shared_ptr<SceneData>)> onPostEffect();
    std::function<void()> onFinishScene();

    // Phase handlers; each reports done to SceneManager under "sound_engine".
    void handleOnSetup(const std::vector<std::shared_ptr<SceneData>>& scenes);
    void handleOnMainRender(std::shared_ptr<SoundData> sound);
    void handleOnPreRender(std::shared_ptr<SoundData> previous, std::shared_ptr<SoundData> current);
    void handleOnPreEffect(std::shared_ptr<SoundData> previous, std::shared_ptr<SoundData> current);
    void handleOnPostEffect(std::shared_ptr<SoundData> sound);
    void handleOnFinishScene();

private:
    SoundEngine() : m_mutedVolumeBgm(1.0f), m_mutedVolumeSe(1.0f) {}

    std::string m_bgmPath;
    std::string m_sePath;
    std::string m_voicePath;
    float m_mutedVolumeBgm;  // volumes remembered by setMute() for resetMute()
    float m_mutedVolumeSe;
    LoadFunction m_load;
    SaveFunction m_save;
    std::function<void(bool)> m_muteChangeListener;
};

#endif
