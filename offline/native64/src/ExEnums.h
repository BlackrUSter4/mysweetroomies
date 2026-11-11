// Every enum the story engine reads from the scenario JSON, plus SceneEventType and AvailableScene, and the
// EnumString tables that map them to/from their JSON spellings.
//
// The numeric values are the original's (recovered from the static initialisers that built the EnumString tables in
// libotome.so 1.4.6); renderers switch on them, so never reorder. The JSON spelling of each value is given next to it.
#ifndef EXENUMS_H
#define EXENUMS_H

#include "EnumString.h"

// scene.fg[].size — which pre-scaled sprite set of a character to use (the directory name under the character's
// perspective: img/resource/character/<app>/<char>/<perspective>/<size>/layer/...).
enum ExScenarioFgSize {
    ExScenarioFgSize_S = 0,  // "s"
    ExScenarioFgSize_M = 1,  // "m"  (default)
    ExScenarioFgSize_L = 2,  // "l"
};

// scene.fg[].position — horizontal slot of a character on screen.
enum ExScenarioFgPosition {
    ExScenarioFgPosition_Left = 0,    // "l"
    ExScenarioFgPosition_Middle = 1,  // "m"  (default)
    ExScenarioFgPosition_Right = 2,   // "r"
};

// scene.link_scene — whether the scene advances on its own once its main render is done.
enum ExSceneLinkScene {
    ExSceneLinkScene_None = 0,  // "0": wait for the player (tap / auto-play timer)
    ExSceneLinkScene_Link = 1,  // "1": SceneManager calls next() 0.25 s after every listener finished MainRender
};

// scene.show_tail — text window style (the speech bubble's tail).
enum ExSceneShowTail {
    ExSceneShowTail_Auto = 0,   // "auto"
    ExSceneShowTail_None = 1,   // "none": narration box (default)
    ExSceneShowTail_Speak = 2,  // "speak": speech bubble
    ExSceneShowTail_Think = 3,  // "think": thought bubble
};

// visual.fade.on — what a fade covers.
enum ExVisualFadeOn {
    ExVisualFadeOn_None = 0,  // "none": no fade (FadeData is reset to null)
    ExVisualFadeOn_Both = 1,  // "both"
    ExVisualFadeOn_Bg = 2,    // "bg"
    ExVisualFadeOn_Fg = 3,    // "fg"
};

// visual.fade.speed — converted to FadeData::duration (seconds) by the parser.
enum ExVisualFadeSpeed {
    ExVisualFadeSpeed_None = 0,    // "none":   0 s
    ExVisualFadeSpeed_Fast = 1,    // "fast":   0.25 s
    ExVisualFadeSpeed_Medium = 2,  // "medium": 1 s
    ExVisualFadeSpeed_Slow = 3,    // "slow":   3 s
};

// visual.fade.type — fade shape.
enum ExVisualFadeType {
    ExVisualFadeType_None = 0,        // "none"
    ExVisualFadeType_Even = 1,        // "even": whole screen at once
    ExVisualFadeType_FromCenter = 2,  // "from_center"
    ExVisualFadeType_ToCenter = 3,    // "to_center"
};

// visual.camera.type — camera pan over the background.
enum ExVisualCameraType {
    ExVisualCameraType_None = 0,       // "none"
    ExVisualCameraType_UpDown = 1,     // "up_down"
    ExVisualCameraType_DownUp = 2,     // "down_up"
    ExVisualCameraType_LeftRight = 3,  // "left_right"
    ExVisualCameraType_RightLeft = 4,  // "right_left"
};

// visual.filter.type — image filter applied to background and/or characters (FilterData::type).
enum ExVisualFilterType {
    ExVisualFilterType_None = 0,       // "none"
    ExVisualFilterType_Sepia = 1,      // "sepia"
    ExVisualFilterType_GrayScale = 2,  // "gray_scale"
    ExVisualFilterType_Invert = 3,     // "invert"
    ExVisualFilterType_Blur = 4,       // "blur" (visual.filter.option = blur radius, FilterData::option)
    ExVisualFilterType_Mosaic = 5,     // "mosaic" (the scene parser never enables it; see parseFilter)
};

// visual.filter.on — what the filter applies to.
enum ExVisualFilterOn {
    ExVisualFilterOn_None = 0,  // "none"
    ExVisualFilterOn_Both = 1,  // "both"
    ExVisualFilterOn_Bg = 2,    // "bg"
    ExVisualFilterOn_Fg = 3,    // "fg"
};

// visual.manpu.location — side of the screen a manpu (comic emotion mark) appears on.
enum ExVisualManpuLocation {
    ExVisualManpuLocation_Right = 0,  // "right" (default)
    ExVisualManpuLocation_Left = 1,   // "left"
};

// visual.shake.type — screen shake strength.
enum ExVisualShakeType {
    ExVisualShakeType_None = 0,    // "none"
    ExVisualShakeType_Soft = 1,    // "soft"
    ExVisualShakeType_Medium = 2,  // "medium"
    ExVisualShakeType_Hard = 3,    // "hard"
};

// visual.end_mark.type — mark shown at the end of a story part.
enum ExVisualEndMarkType {
    ExVisualEndMarkType_None = 0,      // "none"
    ExVisualEndMarkType_Continue = 1,  // "continue": "To be continued"
    ExVisualEndMarkType_End = 2,       // "end"
};

// The phases SceneManager drives every listener through (see SceneManager.h for the full sequence). The names are the
// strings in SceneEventTypeNames; SceneManager uses that table's value list to register one done-flag per phase.
enum SceneEventType {
    SceneEventType_Setup = 0,       // "setup":       once, with all scenes          -> SceneEventListener::onSetup
    SceneEventType_MainRender = 1,  // "main_render": current scene                  -> onMainRender
    SceneEventType_PostEffect = 2,  // "post_effect": previous scene, before leaving -> onPostEffect
    SceneEventType_PreRender = 3,   // "pre_render":  (previous, current)            -> onPreRender
    SceneEventType_PreEffect = 4,   // "pre_effect":  (previous, current)            -> onPreEffect
    SceneEventType_Finish = 5,      // "finish":      story over                     -> onFinishScene
};

// Top-level cocos2d scenes the app can switch to (SceneConnector / BeforeScene::scheduleNextScene).
enum AvailableScene {
    AvailableScene_StartGame = 0,   // "start_game"
    AvailableScene_Adventure = 1,   // "adventure": the story player (ConversationScene)
    AvailableScene_OptionMenu = 2,  // "option_menu"
    AvailableScene_Debug = 3,       // "debug"
    AvailableScene_Notice = 4,      // "notice"
};

// The tables, defined in ExEnums.cpp. Constructed during static initialisation; don't use them from other static
// initialisers.
extern EnumString<ExScenarioFgSize> ExScenarioFgSizeNames;
extern EnumString<ExScenarioFgPosition> ExScenarioFgPositionNames;
extern EnumString<ExSceneLinkScene> ExSceneLinkSceneNames;
extern EnumString<ExSceneShowTail> ExSceneShowTailNames;
extern EnumString<ExVisualFadeOn> ExVisualFadeOnNames;
extern EnumString<ExVisualFadeSpeed> ExVisualFadeSpeedNames;
extern EnumString<ExVisualFadeType> ExVisualFadeTypeNames;
extern EnumString<ExVisualCameraType> ExVisualCameraTypeNames;
extern EnumString<ExVisualFilterType> ExVisualFilterTypeNames;
extern EnumString<ExVisualFilterOn> ExVisualFilterOnNames;
extern EnumString<ExVisualManpuLocation> ExVisualManpuLocationNames;
extern EnumString<ExVisualShakeType> ExVisualShakeTypeNames;
extern EnumString<ExVisualEndMarkType> ExVisualEndMarkTypeNames;
extern EnumString<SceneEventType> SceneEventTypeNames;
extern EnumString<AvailableScene> AvailableSceneNames;

#endif
