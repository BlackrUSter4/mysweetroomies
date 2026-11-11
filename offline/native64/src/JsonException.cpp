#include "JsonException.h"

namespace maid {

// orig 001db294
JsonException::JsonException(const std::string& message)
    : MaidException("Exception: " + ("JsonException:" + message)) {}

// orig 001db1f4
JsonException::~JsonException() {}

}  // namespace maid
