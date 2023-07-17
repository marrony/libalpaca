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

}  // namespace alpaca

#endif  // INCLUDE_TYPES_HPP_
