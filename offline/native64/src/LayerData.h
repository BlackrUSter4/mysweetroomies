// LayerData: one image layer of a character sprite (ViewPersonData::layers). Polymorphic base of
//   OffsetLayerData    - a still image placed with an offset (OffsetLayerData.h)
//   FrameAnimationData - an animated layer (blinking eyes, talking mouth) made of OffsetLayerData frames
// Use instanceof<FrameAnimationData>(layer) / std::dynamic_pointer_cast to tell them apart, as PersonRender does.
#ifndef LAYERDATA_H
#define LAYERDATA_H

#include <memory>
#include <string>

class LayerData {
public:
    virtual ~LayerData();  // orig 001d6cb8

    // Layer name from the offset file / scene JSON ("01_body", "03_eye", "05_mouth", ...).
    std::string name;
    // Image file: an absolute resource-cache path to a .png ("" when the layer has no image of its own; a
    // FrameAnimationData keeps its images in `frames`).
    std::string path;
};

// True when `object` points at a T (false for null). orig 001d50ac (weak, instanceof<T, std::shared_ptr<U>>)
template <typename T, typename U>
bool instanceof(const std::shared_ptr<U> &object)
{
    return dynamic_cast<T *>(object.get()) != nullptr;
}
template <typename T, typename U>
bool instanceof(U *object)
{
    return dynamic_cast<T *>(object) != nullptr;
}

#endif
