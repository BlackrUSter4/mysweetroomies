// ColorString: "#RRGGBB" / "#AARRGGBB" colour strings from the scenario JSON <-> ccColor4B.
#ifndef COLORSTRING_H
#define COLORSTRING_H

#include <string>

#include "cocos2d.h"

class ColorString {
public:
    // "#RRGGBB" (alpha 255) or "#AARRGGBB"; the '#' is optional. Anything shorter than 6 characters, or of another
    // length once the '#' is stripped, is opaque black. Non-hex digits count as 0.
    static cocos2d::ccColor4B toColor(const std::string &text);
    // "#AARRGGBB" order, but each channel printed as a decimal %02d, exactly like the original (so the result does
    // not round-trip through toColor).
    static std::string toString(cocos2d::ccColor4B &color);
};

#endif
