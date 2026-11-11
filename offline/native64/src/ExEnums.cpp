#include "ExEnums.h"

// Contents copied from the original's static initialisers (libotome.so 1.4.6, file offsets 0x189bce..0x18a724 and
// 0x18ba34 for AvailableScene): value lists are 0..n-1 in this order.
EnumString<ExScenarioFgSize> ExScenarioFgSizeNames(
    {ExScenarioFgSize_S, ExScenarioFgSize_M, ExScenarioFgSize_L}, {"s", "m", "l"});

EnumString<ExScenarioFgPosition> ExScenarioFgPositionNames(
    {ExScenarioFgPosition_Left, ExScenarioFgPosition_Middle, ExScenarioFgPosition_Right}, {"l", "m", "r"});

EnumString<ExSceneLinkScene> ExSceneLinkSceneNames({ExSceneLinkScene_None, ExSceneLinkScene_Link}, {"0", "1"});

EnumString<ExSceneShowTail> ExSceneShowTailNames(
    {ExSceneShowTail_Auto, ExSceneShowTail_None, ExSceneShowTail_Speak, ExSceneShowTail_Think},
    {"auto", "none", "speak", "think"});

EnumString<ExVisualFadeOn> ExVisualFadeOnNames(
    {ExVisualFadeOn_None, ExVisualFadeOn_Both, ExVisualFadeOn_Bg, ExVisualFadeOn_Fg}, {"none", "both", "bg", "fg"});

EnumString<ExVisualFadeSpeed> ExVisualFadeSpeedNames(
    {ExVisualFadeSpeed_None, ExVisualFadeSpeed_Fast, ExVisualFadeSpeed_Medium, ExVisualFadeSpeed_Slow},
    {"none", "fast", "medium", "slow"});

EnumString<ExVisualFadeType> ExVisualFadeTypeNames(
    {ExVisualFadeType_None, ExVisualFadeType_Even, ExVisualFadeType_FromCenter, ExVisualFadeType_ToCenter},
    {"none", "even", "from_center", "to_center"});

EnumString<ExVisualCameraType> ExVisualCameraTypeNames(
    {ExVisualCameraType_None, ExVisualCameraType_UpDown, ExVisualCameraType_DownUp, ExVisualCameraType_LeftRight,
     ExVisualCameraType_RightLeft},
    {"none", "up_down", "down_up", "left_right", "right_left"});

EnumString<ExVisualFilterType> ExVisualFilterTypeNames(
    {ExVisualFilterType_None, ExVisualFilterType_Sepia, ExVisualFilterType_GrayScale, ExVisualFilterType_Invert,
     ExVisualFilterType_Blur, ExVisualFilterType_Mosaic},
    {"none", "sepia", "gray_scale", "invert", "blur", "mosaic"});

EnumString<ExVisualFilterOn> ExVisualFilterOnNames(
    {ExVisualFilterOn_None, ExVisualFilterOn_Both, ExVisualFilterOn_Bg, ExVisualFilterOn_Fg},
    {"none", "both", "bg", "fg"});

EnumString<ExVisualManpuLocation> ExVisualManpuLocationNames(
    {ExVisualManpuLocation_Right, ExVisualManpuLocation_Left}, {"right", "left"});

EnumString<ExVisualShakeType> ExVisualShakeTypeNames(
    {ExVisualShakeType_None, ExVisualShakeType_Soft, ExVisualShakeType_Medium, ExVisualShakeType_Hard},
    {"none", "soft", "medium", "hard"});

EnumString<ExVisualEndMarkType> ExVisualEndMarkTypeNames(
    {ExVisualEndMarkType_None, ExVisualEndMarkType_Continue, ExVisualEndMarkType_End}, {"none", "continue", "end"});

EnumString<SceneEventType> SceneEventTypeNames(
    {SceneEventType_Setup, SceneEventType_MainRender, SceneEventType_PostEffect, SceneEventType_PreRender,
     SceneEventType_PreEffect, SceneEventType_Finish},
    {"setup", "main_render", "post_effect", "pre_render", "pre_effect", "finish"});

EnumString<AvailableScene> AvailableSceneNames(
    {AvailableScene_StartGame, AvailableScene_Adventure, AvailableScene_OptionMenu, AvailableScene_Debug,
     AvailableScene_Notice},
    {"start_game", "adventure", "option_menu", "debug", "notice"});
