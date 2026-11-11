#include "SoundEngine.h"

#include "MediaConnector.h"
#include "SceneData.h"
#include "SceneEventListener.h"
#include "SceneManager.h"

namespace {
const std::string LISTENER_NAME = "sound_engine";
const std::string KEY_VOLUME_BGM = LISTENER_NAME + ".volume.bgm";
const std::string KEY_VOLUME_SE = LISTENER_NAME + ".volume.se";
const std::string RESET_PATH = "__RESET__";

float clampVolume(float volume) {
    if (volume >= 1.0f) {
        return 1.0f;
    }
    if (volume <= 0.0f) {
        return 0.0f;
    }
    return volume;
}

std::shared_ptr<SoundData> soundOf(const std::shared_ptr<SceneData>& scene) {
    return scene ? scene->sound : std::shared_ptr<SoundData>();
}
}  // namespace

// orig 001c6f54
SoundEngine* SoundEngine::getInstance() {
    static SoundEngine instance;
    static bool initialized = instance.init();
    (void)initialized;
    return &instance;
}

// orig 001c6f20
SoundEngine::~SoundEngine() {}

// orig 001ccda8
bool SoundEngine::init() {
    setDefaultIO();
    setDefaultMuteChangeListener();
    m_mutedVolumeBgm = 1.0f;
    m_mutedVolumeSe = 1.0f;
    return true;
}

// orig 001ccfec
void SoundEngine::prepare(LoadFunction load, SaveFunction save) {
    std::shared_ptr<SceneEventListener> listener = std::make_shared<SceneEventListener>(
        onSetup(), onMainRender(), onPreRender(), onPreEffect(), onPostEffect(), onFinishScene());
    SceneManager::getInstance()->addListener(LISTENER_NAME, listener);
    setDefaultMuteChangeListener();
    setIO(load, save);
    this->load();
}

// orig 001ccce8
void SoundEngine::release() {
    SceneManager::getInstance()->removeListener(LISTENER_NAME);
    setDefaultIO();
    m_bgmPath = "";
    m_sePath = "";
    m_voicePath = "";
    stop();
    MediaConnector::end();
}

// orig 001cc4ea / 001cc4f4 / 001cc4fe
void SoundEngine::playBgm(const char* path) {
    MediaConnector::playBgm(path);
}

void SoundEngine::playSe(const char* path) {
    MediaConnector::playSe(path);
}

void SoundEngine::playVoice(const char* path) {
    MediaConnector::playVoice(path);
}

// orig 001cc498
void SoundEngine::playBgm() {
    if (!m_bgmPath.empty()) {
        MediaConnector::playBgm(m_bgmPath.c_str());
    }
}

// orig 001cc4c0
void SoundEngine::playSe() {
    if (!m_sePath.empty()) {
        MediaConnector::playSe(m_sePath.c_str());
    }
}

// orig 001cc4ac
void SoundEngine::playVoice() {
    if (!m_voicePath.empty()) {
        MediaConnector::playVoice(m_voicePath.c_str());
    }
}

// orig 001cc4d4
void SoundEngine::play() {
    playVoice();
    playSe();
    playBgm();
}

// orig 001cc50c
void SoundEngine::playSoundData(std::shared_ptr<SoundData> sound) {
    if (!sound->bgm.empty()) {
        if (sound->bgm == RESET_PATH) {
            MediaConnector::stopBgm();
            m_bgmPath = "";
        } else if (m_bgmPath != sound->bgm) {
            MediaConnector::stopBgm();
            MediaConnector::playBgm(sound->bgm.c_str());
            m_bgmPath = sound->bgm;
        }
    }
    m_sePath = sound->se;
    playSe();
    m_voicePath = sound->voice;
    playVoice();
}

// orig 001cc508
void SoundEngine::preloadSoundData(std::shared_ptr<SoundData> sound) {
    (void)sound;
}

// orig 001cc588 / 001cc590 / 001cc598
void SoundEngine::stop() {
    MediaConnector::stop();
}

void SoundEngine::pause() {
    MediaConnector::pause();
}

void SoundEngine::resume() {
    MediaConnector::resume();
}

// orig 001cc5a0 / 001cc5ac / 001cc5ba
std::string SoundEngine::getBgmPath() {
    return m_bgmPath;
}

std::string SoundEngine::getSePath() {
    return m_sePath;
}

std::string SoundEngine::getVoicePath() {
    return m_voicePath;
}

// orig 001cc5c8
void SoundEngine::clearSoundPathBgm() {
    m_bgmPath = "";
}

// orig 001cc5d8
void SoundEngine::clearSoundPathEffect() {
    m_sePath = "";
    m_voicePath = "";
}

// orig 001cc5f8
void SoundEngine::clearSoundPath() {
    clearSoundPathBgm();
    clearSoundPathEffect();
}

// orig 001cc608
float SoundEngine::getVolumeBgm() {
    return MediaConnector::getVolumeBgm();
}

// orig 001cc610
float SoundEngine::getVolumeSe() {
    return MediaConnector::getVolumeSe();
}

// orig 001cc664
void SoundEngine::setVolumeBgm(float volume) {
    float previous = getVolumeBgm();
    volume = clampVolume(volume);
    MediaConnector::setVolumeBgm(volume);
    saveValue(KEY_VOLUME_BGM, volume);
    // Only crossings of 0 change the mute state.
    if (previous != volume && (volume == 0.0f || previous == 0.0f)) {
        notifyMuteChange();
    }
}

// orig 001cc6d4
void SoundEngine::setVolumeSe(float volume) {
    float previous = getVolumeSe();
    volume = clampVolume(volume);
    MediaConnector::setVolumeSe(volume);
    MediaConnector::setVolumeVoice(volume);
    saveValue(KEY_VOLUME_SE, volume);
    if (previous != volume && (volume == 0.0f || previous == 0.0f)) {
        notifyMuteChange();
    }
}

// orig 001cc7ae
float SoundEngine::getDefaultBgmValue() {
    return 1.0f;
}

// orig 001cc7b4
float SoundEngine::getDefaultSeValue() {
    return 1.0f;
}

// orig 001cc618
bool SoundEngine::isMute() {
    float bgm = getVolumeBgm();
    float se = getVolumeSe();
    return bgm <= 0.0f && se <= 0.0f;
}

// orig 001cc74c
void SoundEngine::setMute() {
    m_mutedVolumeBgm = getVolumeBgm();
    m_mutedVolumeSe = getVolumeSe();
    setVolumeBgm(0.0f);
    setVolumeSe(0.0f);
}

// orig 001cc772
void SoundEngine::resetMute() {
    float bgm = getVolumeBgm();
    float se = getVolumeSe();
    if (bgm <= 0.0f) {
        setVolumeBgm(m_mutedVolumeBgm);
    }
    if (se <= 0.0f) {
        setVolumeSe(m_mutedVolumeSe);
    }
}

// orig 001cce3c
void SoundEngine::setMuteChangeListener(std::function<void(bool)> listener) {
    m_muteChangeListener = listener;
}

// orig 001ccd34
void SoundEngine::setDefaultMuteChangeListener() {
    m_muteChangeListener = [](bool) {};
}

// orig 001cc644
void SoundEngine::notifyMuteChange() {
    bool mute = isMute();
    m_muteChangeListener(mute);
}

// orig 001ccdc4
void SoundEngine::setIO(LoadFunction load, SaveFunction save) {
    m_load = load;
    m_save = save;
}

// orig 001ccc34
void SoundEngine::setDefaultIO() {
    m_load = [](const std::string&, float) { return 1.0f; };
    m_save = [](const std::string&, float) {};
}

// orig 001cc870
float SoundEngine::loadValue(const std::string& key, float defaultValue) {
    return m_load(key, defaultValue);
}

// orig 001cc314
void SoundEngine::saveValue(const std::string& key, float value) {
    m_save(key, value);
}

// orig 001cc87c
void SoundEngine::load() {
    float bgm = loadValue(KEY_VOLUME_BGM, 1.0f);
    float se = loadValue(KEY_VOLUME_SE, 1.0f);
    setVolumeBgm(bgm);
    setVolumeSe(se);
}

// orig 001cc32c (lambda body 001cc940)
std::function<void(const std::vector<std::shared_ptr<SceneData>>&)> SoundEngine::onSetup() {
    return [this](const std::vector<std::shared_ptr<SceneData>>& scenes) { handleOnSetup(scenes); };
}

// orig 001cc380 (lambda body 001ccb7c)
std::function<void(std::shared_ptr<SceneData>)> SoundEngine::onMainRender() {
    return [this](std::shared_ptr<SceneData> scene) { handleOnMainRender(scene->sound); };
}

// orig 001cc3f0 (lambda body 001cca3c)
std::function<void(std::shared_ptr<SceneData>, std::shared_ptr<SceneData>)> SoundEngine::onPreRender() {
    return [this](std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current) {
        handleOnPreRender(soundOf(previous), current->sound);
    };
}

// orig 001cc3b8 (lambda body 001cc964)
std::function<void(std::shared_ptr<SceneData>, std::shared_ptr<SceneData>)> SoundEngine::onPreEffect() {
    return [this](std::shared_ptr<SceneData> previous, std::shared_ptr<SceneData> current) {
        handleOnPreEffect(soundOf(previous), current->sound);
    };
}

// orig 001cc428 (lambda body 001ccaf0)
std::function<void(std::shared_ptr<SceneData>)> SoundEngine::onPostEffect() {
    return [this](std::shared_ptr<SceneData> scene) { handleOnPostEffect(scene->sound); };
}

// orig 001cc460 (lambda body 001ccbf0)
std::function<void()> SoundEngine::onFinishScene() {
    return [this]() { handleOnFinishScene(); };
}

// orig 001cc8bc
void SoundEngine::handleOnSetup(const std::vector<std::shared_ptr<SceneData>>& scenes) {
    for (size_t i = 0; i < scenes.size(); ++i) {
        std::shared_ptr<SceneData> scene = scenes[i];
        if (scene && scene->sound) {
            preloadSoundData(scene->sound);
        }
    }
    SceneManager::getInstance()->doneListenerEvent(SceneEventType_Setup, LISTENER_NAME);
}

// orig 001ccb40
void SoundEngine::handleOnMainRender(std::shared_ptr<SoundData> sound) {
    playSoundData(sound);
    SceneManager::getInstance()->doneListenerEvent(SceneEventType_MainRender, LISTENER_NAME);
}

// orig 001cc9f8
void SoundEngine::handleOnPreRender(std::shared_ptr<SoundData> previous, std::shared_ptr<SoundData> current) {
    (void)previous;
    clearSoundPathEffect();
    preloadSoundData(current);
    SceneManager::getInstance()->doneListenerEvent(SceneEventType_PreRender, LISTENER_NAME);
}

// orig 001cc94c
void SoundEngine::handleOnPreEffect(std::shared_ptr<SoundData> previous, std::shared_ptr<SoundData> current) {
    (void)previous;
    (void)current;
    SceneManager::getInstance()->doneListenerEvent(SceneEventType_PreEffect, LISTENER_NAME);
}

// orig 001ccad0
void SoundEngine::handleOnPostEffect(std::shared_ptr<SoundData> sound) {
    (void)sound;
    MediaConnector::stopVoice();
    MediaConnector::stopSe();
    SceneManager::getInstance()->doneListenerEvent(SceneEventType_PostEffect, LISTENER_NAME);
}

// orig 001ccbcc
void SoundEngine::handleOnFinishScene() {
    stop();
    clearSoundPath();
    SceneManager::getInstance()->doneListenerEvent(SceneEventType_Finish, LISTENER_NAME);
}
