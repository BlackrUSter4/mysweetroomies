// OffsetLayerData: a still layer image plus its offset from the offset file (ExOffsetData::offset).
#ifndef OFFSETLAYERDATA_H
#define OFFSETLAYERDATA_H

#include "LayerData.h"
#include "cocos2d.h"

class OffsetLayerData : public LayerData {
public:
    OffsetLayerData() {}
    // `imagePath` goes to LayerData::path; name stays "". orig 001d6d1c
    OffsetLayerData(const char *imagePath, float offsetX, float offsetY);
    virtual ~OffsetLayerData();  // orig 001d6cdc

    // Offset of the image centre in pixels, as read from the offset file (y grows downwards there).
    float x = 0.0f;
    float y = 0.0f;

    // Anchor point to give a sprite of `imageSize` so that, placed at the person's position, it lands where the offset
    // file says. Coordinates are relative to a 480 px reference square and already have 0.5 subtracted, i.e.
    //   ((w/2 + x) / 480 - 0.5,  (1 - (h/2 + y) / 480) - 0.5)
    // exactly as the original computed it. orig 001d6d5c
    cocos2d::CCPoint getOffsetPoint(cocos2d::CCSize imageSize);
    // Same, measuring the image at `path` (creates a throwaway CCSprite). orig 001d6dd0
    cocos2d::CCPoint getOffsetPoint();
};

#endif
