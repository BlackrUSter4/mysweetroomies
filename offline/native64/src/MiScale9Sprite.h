#ifndef MISCALE9SPRITE_H
#define MISCALE9SPRITE_H

#include "GUI/CCControlExtension/CCScale9Sprite.h"
#include "cocos2d.h"

// Builds a CCScale9Sprite from an Android-style .9.png: the 1-pixel black guide lines on the top and left edges
// give the stretchable centre, the border itself is cut away. Only static helpers; never instantiated.
class MiScale9Sprite {
public:
    static cocos2d::extension::CCScale9Sprite* create(const char* file);

    // Cap-inset rectangle read from the guide pixels of `file`, in image coordinates including the border.
    static cocos2d::CCRect get9PatchRect(const char* file);
};

#endif
