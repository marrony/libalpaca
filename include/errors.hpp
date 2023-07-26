// Copyright (C) 2023 Marrony Neris

#ifndef INCLUDE_ERRORS_HPP_
#define INCLUDE_ERRORS_HPP_

#include <string>

namespace alpaca {

namespace error {

struct alpaca_error {
  const int error_number;
  const std::string error_message;

  alpaca_error(int error_number, const std::string& error_message)
  : error_number(error_number)
  , error_message(error_message)
  { }
};

// reserved error code (0x400) for property or method not implemented.
struct not_implemented : alpaca_error {
  not_implemented()
  : alpaca_error(0x0400, "") { }
};

// reserved error code (0x401) for reporting an invalid value.
struct invalid_value : alpaca_error {
  invalid_value()
  : alpaca_error(0x0401, "")
  { }
};

// reserved error code (0x402) for reporting that a value has not been set.
struct value_not_set : alpaca_error {
  value_not_set()
  : alpaca_error(0x0402, "")
  { }
};

// reserved error code (0x407) used to indicate that the communications
// channel is not connected.
struct not_connected : alpaca_error {
  not_connected()
  : alpaca_error(0x0407, "Not connected")
  { }
};

// reserved error code (0x408) used to indicate that the attempted operation
// is invalid because the mount is currently in a Parked state.
struct parked : alpaca_error {
  parked()
  : alpaca_error(0x0408, "")
  { }
};

// reserved error code (0x409) used to indicate that the attempted
// operation is invalid because the mount is currently in a Slaved state.
struct slaved : alpaca_error {
  slaved()
  : alpaca_error(0x0409, "")
  { }
};

// reserved error code (0x40B) to indicate that the requested
// operation can not be undertaken at this time.
struct invalid_operation : alpaca_error {
  invalid_operation()
  : alpaca_error(0x040B, "")
  { }
};

// reserved error code (0x40C) to indicate that the requested
// action is not implemented in this driver.
struct action_not_implemented : alpaca_error {
  action_not_implemented()
  : alpaca_error(0x040C, "")
  { }
};

// [0x500 - 0xfff] reserved for driver specific errors.

}  // namespace error
}  // namespace alpaca

#endif  // INCLUDE_ERRORS_HPP_
