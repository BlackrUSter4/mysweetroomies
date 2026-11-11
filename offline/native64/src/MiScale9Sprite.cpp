#include "MiScale9Sprite.h"

USING_NS_CC;
USING_NS_CC_EXT;

namespace {

// orig 001dc7b8 check9PatchPixel (a weak global in the original, only used here).
// Looks at one border pixel while walking towards the guide line: transparent pixels advance `counter` by `step`
// and keep the scan going; the black guide pixel, or anything else that is not transparent, ends it.
bool check9PatchPixel(int& counter, int step, const unsigned char* data, int offset) {
    const unsigned char* pixel = data + offset;
    bool opaqueBlack = pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0 && pixel[3] == 0xff;
    if (opaqueBlack) return true;
    if (pixel[3] != 0) return true;
    counter += step;
    return false;
}

}  // namespace

// orig 001dc7e4
CCRect MiScale9Sprite::get9PatchRect(const char* file) {
    CCImage image;
    image.initWithImageFile(file, CCImage::kFmtPng);

    const int width = image.getWidth();
    const int height = image.getHeight();
    const unsigned char* data = image.getData();
    // The pixel arithmetic always reads byte 3 as alpha; RGB images (3 bytes per pixel) are read the same way, as
    // the original did.
    const int bytesPerPixel = image.hasAlpha() ? 4 : 3;
    const int rowBytes = width * bytesPerPixel;

    int top = 0, bottom = height - 1, left = 0, right = width - 1;
    if (!data) return CCRect(0, 0, 0, 0);

    // Left edge, top to bottom and bottom to top: the vertical stretch range.
    for (int y = 0; y < height; ++y)
        if (check9PatchPixel(top, 1, data, y * rowBytes)) break;
    for (int y = height - 1; y >= 0; --y)
        if (check9PatchPixel(bottom, -1, data, y * rowBytes)) break;
    // Top edge, left to right and right to left: the horizontal stretch range.
    // Deviation: the original bounded the left-to-right scan by the image height (reading into the next row of a
    // wide image without a guide line); the width is used here.
    for (int x = 0; x < width; ++x)
        if (check9PatchPixel(left, 1, data, x * bytesPerPixel)) break;
    for (int x = width - 1; x >= 0; --x)
        if (check9PatchPixel(right, -1, data, x * bytesPerPixel)) break;

    return CCRect(static_cast<float>(left), static_cast<float>(top), static_cast<float>(right - left + 1),
                  static_cast<float>(bottom - top + 1));
}

// orig 001dc90c — not called anywhere in the shipped game.
CCScale9Sprite* MiScale9Sprite::create(const char* file) {
    CCTexture2D* texture = CCTextureCache::sharedTextureCache()->addImage(file);
    if (!texture) return nullptr;
    const int pixelsHigh = static_cast<int>(texture->getPixelsHigh());
    const int pixelsWide = static_cast<int>(texture->getPixelsWide());

    CCRect capInsets = get9PatchRect(file);
    capInsets.origin.x += 1.0f;
    capInsets.origin.y += 1.0f;

    // Kept as the original wrote it: the frame skips the 1-pixel guide border at the origin but its width is
    // taken from the pixel height and its height from the pixel width (identical for the square images 9-patches
    // usually are).
    CCRect frameRect(1.0f, 1.0f, static_cast<float>(pixelsHigh - 1), static_cast<float>(pixelsWide - 1));
    CCSpriteFrame* frame = CCSpriteFrame::createWithTexture(texture, frameRect);
    return CCScale9Sprite::createWithSpriteFrame(frame, capInsets);
}
