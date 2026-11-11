#include "FilterData.h"

#include <cstdlib>
#include <sstream>

#include "BaseFilter.h"
#include "BlurFilter.h"
#include "GrayScaleFilter.h"
#include "InvertFilter.h"
#include "MediaConnector.h"
#include "MosaicFilter.h"
#include "NoFilter.h"
#include "ResourceManager.h"
#include "SepiaFilter.h"

USING_NS_CC;

// orig 001d6984
bool FilterData::hasFilter()
{
    return type != ExVisualFilterType_None;
}

// orig 001d699c
int FilterData::getOption()
{
    return option;
}

// orig 001d698c
FilterData &FilterData::setOption(const std::string &value)
{
    option = atoi(value.c_str());
    return *this;
}

// orig 001d6c98
std::string FilterData::getName()
{
    return ExVisualFilterTypeNames.toString(type);
}

// orig 001d69f0
void FilterData::getFilterImagePath(std::string &out, const std::string &sourcePath)
{
    // The original cut exactly four characters (".png") without checking; a shorter path just yields the suffix.
    const std::string stem = sourcePath.size() >= 4 ? sourcePath.substr(0, sourcePath.size() - 4) : std::string();
    std::ostringstream path;
    path << stem << "-" << getName() << "-" << getOption() << ".png";
    out = path.str();
}

// orig 001d6aac
BaseFilter *FilterData::createFilter(const std::string &imagePath)
{
    // A previous scene may have cached this path with another filter applied to the same texture.
    CCTextureCache::sharedTextureCache()->removeTextureForKey(imagePath.c_str());

    BaseFilter *filter = nullptr;
    std::string filteredPath;
    switch (type) {
    case ExVisualFilterType_None:
        filter = BaseFilter::create(imagePath);
        break;
    case ExVisualFilterType_Sepia:
        getFilterImagePath(filteredPath, imagePath);
        filter = ResourceManager::getInstance()->isExistFile(filteredPath) ? NoFilter::create(filteredPath)
                                                                           : SepiaFilter::create(imagePath);
        break;
    case ExVisualFilterType_GrayScale:
        getFilterImagePath(filteredPath, imagePath);
        filter = ResourceManager::getInstance()->isExistFile(filteredPath) ? NoFilter::create(filteredPath)
                                                                           : GrayScaleFilter::create(imagePath);
        break;
    case ExVisualFilterType_Invert:
        getFilterImagePath(filteredPath, imagePath);
        filter = ResourceManager::getInstance()->isExistFile(filteredPath) ? NoFilter::create(filteredPath)
                                                                           : InvertFilter::create(imagePath);
        break;
    case ExVisualFilterType_Blur:
        // The pre-blurred copy still goes through the blur shader, as in the original (blurred twice).
        getFilterImagePath(filteredPath, imagePath);
        filter = BlurFilter::create(ResourceManager::getInstance()->isExistFile(filteredPath) ? filteredPath
                                                                                              : imagePath);
        break;
    case ExVisualFilterType_Mosaic:
        filter = MosaicFilter::create(imagePath);
        break;
    }
    // The original called prepare() unconditionally (and crashed on a missing image or an unknown type).
    if (filter != nullptr) filter->prepare();
    return filter;
}

// orig 001d6bec
bool FilterData::generateFilteredImage(const std::string &imagePath)
{
    const std::string source = imagePath;
    std::string filteredPath;
    getFilterImagePath(filteredPath, source);
    if (ResourceManager::getInstance()->isExistFile(filteredPath)) return true;

    // Blur radius handed to Java: option 0 -> 7 px, option 1 -> 15 px, anything else 0.
    float blurRadius = 0.0f;
    if (getOption() == 0)
        blurRadius = 7.0f;
    else if (getOption() == 1)
        blurRadius = 15.0f;

    switch (type) {
    case ExVisualFilterType_Sepia:
        return MediaConnector::sepia(source.c_str(), filteredPath.c_str());
    case ExVisualFilterType_GrayScale:
        return MediaConnector::grayscale(source.c_str(), filteredPath.c_str());
    case ExVisualFilterType_Invert:
        return MediaConnector::invert(source.c_str(), filteredPath.c_str());
    case ExVisualFilterType_Blur:
        return MediaConnector::blur(source.c_str(), filteredPath.c_str(), blurRadius);
    default:
        return true;
    }
}

// orig 001d6924
CCSpriteFrame *FilterData::prepareFilter(CCSpriteFrame *frame)
{
    CCTextureCache::sharedTextureCache()->removeTexture(frame->getTexture());
    BaseFilter *filter = nullptr;
    switch (type) {
    case ExVisualFilterType_Sepia:
        filter = SepiaFilter::createWithSpriteFrame(frame);
        break;
    case ExVisualFilterType_GrayScale:
        filter = GrayScaleFilter::createWithSpriteFrame(frame);
        break;
    case ExVisualFilterType_Invert:
        filter = InvertFilter::createWithSpriteFrame(frame);
        break;
    case ExVisualFilterType_Blur:
        filter = BlurFilter::createWithSpriteFrame(frame);
        break;
    case ExVisualFilterType_Mosaic:
        filter = MosaicFilter::createWithSpriteFrame(frame);
        break;
    default:
        return frame;
    }
    if (filter != nullptr) filter->prepare();
    return frame;
}
