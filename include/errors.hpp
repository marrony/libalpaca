// Copyright (C) 2023 Marrony Neris

#ifndef INCLUDE_ERRORS_HPP_
#define INCLUDE_ERRORS_HPP_

#include <string>
#include "c++util.hpp"

namespace alpaca {

struct alpaca_error {
  int error_number;
  std::string error_message;
};

// reserved error code (0x400) for property or method not implemented.
auto not_implemented() {
  return unexpected(alpaca_error{0x0400, "Not implemented"});
}

// reserved error code (0x401) for reporting an invalid value.
auto invalid_value() {
  return unexpected(alpaca_error{0x0401, "Invalid value"});
}

// reserved error code (0x402) for reporting that a value has not been set.
auto value_not_set() {
  return unexpected(alpaca_error{0x0402, "Value not set"});
}

// reserved error code (0x407) used to indicate that the communications
// channel is not connected.
auto not_connected() {
  return unexpected(alpaca_error{0x0407, "Not connected"});
}

// reserved error code (0x408) used to indicate that the attempted operation
// is invalid because the mount is currently in a Parked state.
auto parked() {
  return unexpected(alpaca_error{0x0408, "Parked"});
}

// reserved error code (0x409) used to indicate that the attempted
// operation is invalid because the mount is currently in a Slaved state.
auto slaved() {
  return unexpected(alpaca_error{0x0409, "Slaved"});
}

// reserved error code (0x40B) to indicate that the requested
// operation can not be undertaken at this time.
auto invalid_operation() {
  return unexpected(alpaca_error{0x040B, "Invalid operation"});
}

// reserved error code (0x40C) to indicate that the requested
// action is not implemented in this driver.
auto action_not_implemented() {
  return unexpected(alpaca_error{0x040C, "Action not implemented"});
}

// [0x500 - 0xfff] reserved for driver specific errors.
auto custom_error(const std::string& str) {
  return unexpected(alpaca_error{0x500, str});
}

}  // namespace alpaca

#endif  // INCLUDE_ERRORS_HPP_
