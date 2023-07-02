// Copyright (C) 2023 Marrony Neris

#ifndef INCLUDE_TYPES_HPP_
#define INCLUDE_TYPES_HPP_

#include <map>
#include <string_view>

#include <util.hpp>

namespace alpaca {

struct comparator_t {
  std::function<bool(const std::string&, const std::string&)> fn;

  bool operator()(const std::string& a, const std::string& b) const {
    return fn(a, b);
  }
};

using arguments_t = std::map<
  std::string,
  std::string,
  comparator_t
>;

enum class driver_rate_t : int {
  sidereal = 0,
  lunar,
  solar,
  king
};

struct driver_rate_arcs_t {
  const float sidereal = 15.0410;
  const float lunar    = 14.6850;
  const float solar    = 15.0000;
  const float king     = 15.0369;
};

struct axis_rate_t {
  float minimum;
  float maximum;
};

enum class equatorial_system_t : std::int32_t {
  other = 0,
  jnow  = 1,
  j2000 = 2,
  j2050 = 3,
  b1950 = 4
};

enum class alignment_mode_t : std::int32_t {
  alt_azm = 0,
  polar   = 1,
  german  = 2
};

enum class destination_side_of_pier_t : std::int32_t {
  pier_unknown = -1,
  pier_east    = 0,
  pier_west    = 1,
};

}  // namespace alpaca

#endif  // INCLUDE_TYPES_HPP_
