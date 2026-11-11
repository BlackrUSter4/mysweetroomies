#ifndef MAID_JSONEXCEPTION_H
#define MAID_JSONEXCEPTION_H

#include "MaidException.h"

namespace maid {

// Thrown by the maid::*PreferenceData classes on malformed JSON or a type mismatch.
// what() == "Exception: JsonException:" + message.
class JsonException : public MaidException {
public:
    explicit JsonException(const std::string& message);
    virtual ~JsonException();
};

}  // namespace maid

#endif
