// The game ships only PNGs, so the arm64 build leaves libwebp out; this replaces CCImageCommonWebp.cpp.
#include "platform/CCImage.h"
NS_CC_BEGIN
bool CCImage::_initWithWebpData(void *pData, int nDataLen) { CC_UNUSED_PARAM(pData); CC_UNUSED_PARAM(nDataLen); return false; }
NS_CC_END
