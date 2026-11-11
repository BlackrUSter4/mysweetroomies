#include "FrameAnimationData.h"

#include "BaseFilter.h"
#include "FilterData.h"

USING_NS_CC;

FrameAnimationData::~FrameAnimationData() {}

CCAnimation *FrameAnimationData::toCCAnimation(FilterData &filter, int loops)
{
    const int steps = (int)sequence.size();
    CCAnimation *animation = CCAnimation::create();

    // Filtered frames are pre-rendered once per distinct image, then referenced by index.
    std::vector<CCSpriteFrame *> filteredFrames;
    if (filter.hasFilter()) {
        for (size_t i = 0; i < frames.size(); ++i) {
            BaseFilter *filtered = filter.createFilter(frames[i]->path);
            filteredFrames.push_back(CCSpriteFrame::createWithTexture(filtered->getTexture(),
                                                                      filtered->getTextureRect()));
        }
    }

    for (int i = 0; i < steps; ++i) {
        int index = sequence[i];
        if (index < 0 || index >= (int)frames.size() || frames[index]->path.empty()) continue;
        if (filteredFrames.empty())
            animation->addSpriteFrameWithFileName(frames[index]->path.c_str());
        else
            animation->addSpriteFrame(filteredFrames.at(index));
    }

    animation->setDelayPerUnit(duration / (float)steps);
    animation->setRestoreOriginalFrame(true);
    animation->setLoops(loops);
    return animation;
}
