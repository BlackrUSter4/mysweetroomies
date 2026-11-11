// FilterData: the image filter of a background or character (ViewBackgroundData::filter, ViewPersonData::filter),
// copied around by value inside SceneData.
//
// A filtered image is drawn in one of two ways:
//   * pre-rendered: SceneDataParser calls generateFilteredImage(path) for every image of the scene, which asks the
//     Java side (MediaConnector::sepia/grayscale/invert/blur) to write "<path minus .png>-<filter>-<option>.png".
//     createFilter() then loads that file as a plain NoFilter sprite (blur: as a BlurFilter on the blurred file).
//   * live shader: when the pre-rendered file is missing, createFilter() loads the original image into a
//     SepiaFilter/GrayScaleFilter/... sprite whose GL program does the work.
#ifndef FILTERDATA_H
#define FILTERDATA_H

#include <string>

#include "ExEnums.h"
#include "cocos2d.h"

class BaseFilter;

class FilterData {
public:
    // Public because SceneData holds FilterData by value and SceneDataParser's parseFilter() sets `type` directly,
    // as the original did.
    ExVisualFilterType type = ExVisualFilterType_None;
    int option = 0;  // visual.filter.option (blur radius); always 0 for the other filters

    bool hasFilter();  // type != None
    int getOption();
    FilterData &setOption(const std::string &value);  // atoi(value); returns *this like the original
    std::string getName();                            // JSON name of `type` ("sepia", "gray_scale", ...)

    // out = "<sourcePath without its last 4 characters>-<getName()>-<option>.png"
    void getFilterImagePath(std::string &out, const std::string &sourcePath);

    // Autoreleased sprite of `imagePath` with the filter applied and prepare()d (see the header comment). With
    // type None it is a BaseFilter without a shader. Null if the image could not be loaded.
    BaseFilter *createFilter(const std::string &imagePath);

    // Asks Java to pre-render the filtered copy of `imagePath` unless it already exists. Returns what the Java helper
    // returned, or true when nothing had to be done.
    bool generateFilteredImage(const std::string &imagePath);

    // Drops the frame's texture from the texture cache and runs prepare() on a throwaway filter sprite built from the
    // frame. Returns `frame` itself. No caller in the game.
    cocos2d::CCSpriteFrame *prepareFilter(cocos2d::CCSpriteFrame *frame);
};

#endif
