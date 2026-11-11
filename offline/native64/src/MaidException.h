#ifndef MAID_MAIDEXCEPTION_H
#define MAID_MAIDEXCEPTION_H

#include <stdexcept>
#include <string>

namespace maid {

// Base of the engine's exceptions. what() carries the full text built by the subclass,
// e.g. "Exception: IOException:cannot load file: /data/...".
class MaidException : public std::runtime_error {
public:
    explicit MaidException(const std::string& message) : std::runtime_error(message) {}
    virtual ~MaidException();
};

}  // namespace maid

#endif
