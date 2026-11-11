#include "ColorString.h"

#include <cstdio>

USING_NS_CC;

namespace {

int hexDigitValue(unsigned char c)
{
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= '0' && c <= '9') return c - '0';
    return 0;  // the original folds every other character into a 0 digit instead of rejecting the string
}

}  // namespace

// orig 001dbea4
ccColor4B ColorString::toColor(const std::string &text)
{
    // The length check runs before the '#' is stripped, so "#12345" passes it and then fails the 6/8 test below.
    if (text.size() < 6) return ccc4(0, 0, 0, 255);

    std::string digits = text[0] == '#' ? text.substr(1) : text;
    // Unsigned on purpose: eight digits fill all 32 bits, and the original shifted past them modulo 2^32.
    unsigned int value = 0;
    for (unsigned char c : digits) value = value * 16 + hexDigitValue(c);

    if (digits.size() == 6) return ccc4((value >> 16) & 0xff, (value >> 8) & 0xff, value & 0xff, 255);
    if (digits.size() == 8)
        return ccc4((value >> 16) & 0xff, (value >> 8) & 0xff, value & 0xff, (value >> 24) & 0xff);
    return ccc4(0, 0, 0, 255);
}

// orig 001dbfb0
std::string ColorString::toString(ccColor4B &color)
{
    char buffer[16];
    // %02d (decimal) is what the original printed; kept so callers see the same text. Each channel is at most
    // 3 digits, so 1 + 4 * 3 + NUL fits.
    snprintf(buffer, sizeof(buffer), "#%02d%02d%02d%02d", color.a, color.r, color.g, color.b);
    return buffer;
}
