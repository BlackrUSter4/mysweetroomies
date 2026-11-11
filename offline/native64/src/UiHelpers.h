// UiHelpers: the global helpers of the original's ui translation units (title / notice / debug screens, the options
// dialog and the in-story overlay menu). The artwork is laid out for a 640-pixel-wide design screen; these scale it
// to the real window width.
//
// Not here, although helpers.c lists them next to these:
//   - check9PatchPixel (orig 001dc7b8) is private to MiScale9Sprite.cpp (only caller), defined there.
//   - getSpriteScale / getTextUpperBound / getCursorAction / getTextWindowBackgroundPath live in TextHelpers.h.
//   - updateMaxSize / resetCurtainAction belong to CanvasLayer (render).
#ifndef UIHELPERS_H
#define UIHELPERS_H

#include "cocos2d.h"

class OptionsDialog;
// Child tags of OptionsDialog; setDefaultValue() takes one of the slider tags. Defined in OptionsDialog.h.
enum OptionsDialogTag : int;

// orig 001c9d1c: node->setScale(winWidth / 640) — design pixels to screen pixels.
void scalingNode(cocos2d::CCNode* node);

// orig 001ca7e8: scale so the node is `ratio` times as wide as the window (winWidth / contentWidth * ratio).
void matchParentWidth(cocos2d::CCNode* node, float ratio);

// orig 001cb1b0: node->setScale(winWidth / 640 * ratio), for nodes without a meaningful content size (particles).
void matchParentWidthFromCanvasSize(cocos2d::CCNode* node, float ratio);

// orig 001cb0f0: the button click of the title / debug screens.
void playButtonSe();

// orig 001e12b4 / 001e1314: overlay-menu open and close clicks.
void playMenuInSE();
void playMenuOutSE();

// orig 001e0f10: text-speed slider position (0..1) to auto-play delay: 0.25 s at 0, 2 * AUTOPLAY_DELAY - 0.25 s
// at 1 (8.25 s with the 4.25 s default). Stored through SceneManager::setAutoPlayDelay().
void setAutoTextValueFromSlider(float sliderValue);

// orig 001e0f7a: puts one OptionsDialog slider back to its default (BGM / SE: SoundEngine defaults, text speed: 0.5)
// and applies it.
void setDefaultValue(OptionsDialog* dialog, OptionsDialogTag tag);

#endif
