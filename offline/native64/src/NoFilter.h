// NoFilter: a BaseFilter that needs no shader because its image file is already filtered - FilterData::createFilter
// uses it for the "<image>-<filter>-<option>.png" copies the Java side pre-rendered.
#ifndef NOFILTER_H
#define NOFILTER_H

#include <string>

#include "BaseFilter.h"

class NoFilter : public BaseFilter {
public:
    virtual ~NoFilter();

    static BaseFilter *create(const std::string &imagePath);  // autoreleased; null if the image cannot be loaded
    virtual void prepare();                                   // nothing to do
};

#endif
