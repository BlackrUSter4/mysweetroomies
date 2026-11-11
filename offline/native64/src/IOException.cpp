#include "IOException.h"

namespace maid {

// orig 001cd7c8
IOException::IOException(const std::string& message) : MaidException("Exception: " + ("IOException:" + message)) {}

// orig 001cd1d4
IOException::~IOException() {}

}  // namespace maid
