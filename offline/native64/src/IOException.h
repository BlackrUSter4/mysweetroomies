#ifndef MAID_IOEXCEPTION_H
#define MAID_IOEXCEPTION_H

#include "MaidException.h"

namespace maid {

// Thrown by ResourceManager when an absolute path cannot be opened. what() == "Exception: IOException:" + message.
class IOException : public MaidException {
public:
    explicit IOException(const std::string& message);
    virtual ~IOException();
};

}  // namespace maid

#endif
