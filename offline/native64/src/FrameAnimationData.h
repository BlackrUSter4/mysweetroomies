// FrameAnimationData: an animated character layer (blinking eyes "03_eye", talking mouth "05_mouth"), built by
// SceneDataParser from a layer whose JSON value is an array of images.
#ifndef FRAMEANIMATIONDATA_H
#define FRAMEANIMATIONDATA_H

#include <memory>
#include <vector>

#include "LayerData.h"
#include "OffsetLayerData.h"
#include "cocos2d.h"

class FilterData;

class FrameAnimationData : public LayerData {
public:
    virtual ~FrameAnimationData();  // orig 001d75a0

    // Length of one pass through `sequence`, in seconds (every entry gets duration / sequence.size()).
    float duration = 0.0f;
    // GUESS: a 4-byte field the original zero-initialises and never reads or writes.
    int unused = 0;
    // The distinct images, in JSON order. Their offsets/paths are filled like any still layer.
    std::vector<std::shared_ptr<OffsetLayerData>> frames;
    // Frame indices to play in order (into `frames`); out-of-range indices and frames without a path are skipped.
    std::vector<int> sequence;

    // Builds an autoreleased CCAnimation playing `sequence` `loops` times (restoreOriginalFrame = true). With an
    // active filter every frame is first rendered through FilterData::createFilter. orig 001d4af4
    cocos2d::CCAnimation *toCCAnimation(FilterData &filter, int loops);
};

#endif
