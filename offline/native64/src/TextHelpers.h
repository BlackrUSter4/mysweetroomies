// TextHelpers: the global helpers of the original's TextWindowLayer translation unit (orig 001e2648..001e277c).
// Only TextWindowLayer uses them. All lengths are in the 640-pixel-wide design space times getSpriteScale().
#ifndef TEXTHELPERS_H
#define TEXTHELPERS_H

#include <string>

#include "ExEnums.h"
#include "cocos2d.h"

// Horizontal scale of the 640-pixel-wide artwork on this screen: winSize.width / 640.
float getSpriteScale();

// (0, winHeight / 2 - winWidth): where the top of the text window sits, in the coordinates of a layer whose origin
// is the screen centre. The window is a square-ish area winWidth tall at the bottom of the screen.
cocos2d::CCPoint getTextUpperBound();

// "Tap to continue" cursor bounce: fade to fully opaque instantly, then move 10 px up and back (0.6 s each way)
// forever. `position` is the cursor's resting point.
cocos2d::CCAction *getCursorAction(cocos2d::CCPoint &position);

// Auto-play cursor blink: fade out and in (0.75 s each) forever. The original has no symbol for it (orig 001e2648).
cocos2d::CCAction *getAutoPlayCursorAction();

// Picks the frame name for a window style from a three-entry table {none/auto, speak, think}: Speak -> [1],
// Think -> [2], anything else -> [0].
std::string getTextWindowBackgroundPath(const std::string *paths, ExSceneShowTail showTail);

#endif
