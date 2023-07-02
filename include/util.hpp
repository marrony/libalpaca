// Copyright (C) 2023 Marrony Neris

#ifndef INCLUDE_UTIL_HPP_
#define INCLUDE_UTIL_HPP_

#include <sys/time.h>
#include <vector>
#include <cctype>
#include <string>

#define NO_OP
#define LOWER std::tolower

#define COMPARATOR(x, y, op) { \
    size_t l1 = (x).size(); \
    size_t l2 = (y).size(); \
    if (l1 < l2) return true; \
    if (l1 > l2) return false; \
    \
    for (size_t n = 0; n < l1; n++) { \
        int xc = op((x)[n]); \
        int yc = op((y)[n]); \
        if (xc < yc) return true; \
        if (xc > yc) return false; \
    } \
    return false; \
}

namespace alpaca {
namespace util {

static inline std::string to_lower(std::string_view str) {
  std::string out(str.size(), '\0');
  std::transform(
    str.begin(),
    str.end(),
    out.begin(),
    [](unsigned char c) { return std::tolower(c); });
  return out;
}

static inline bool compare_less_insensitive(std::string_view a, std::string_view b) {
  COMPARATOR(a, b, LOWER);
}

static inline bool compare_less_sensitive(std::string_view a, std::string_view b) {
  COMPARATOR(a, b, NO_OP);
}

static inline bool equals_insentive(std::string_view x, std::string_view y) {
  size_t l1 = x.size();
  size_t l2 = y.size();

  if (l1 != l2) return false;

  for (size_t n = 0; n < l1; n++) {
    int xc = std::tolower(x[n]);
    int yc = std::tolower(y[n]);

    if (xc != yc) return false;
  }

  return true;
}

static inline std::vector<std::string_view> split(std::string_view str, std::string_view delim) {
  std::vector<std::string_view> tokens;

  size_t last_pos = 0;
  size_t pos = 0;
  while ((pos = str.find(delim, last_pos)) != std::string::npos) {
    tokens.push_back(str.substr(last_pos, pos - last_pos));
    last_pos = pos + 1;
  }

  tokens.push_back(str.substr(last_pos));

  return tokens;
}

static inline int parse_int(std::string_view str, int default_value) {
  try {
    return std::stoi(std::string(str));
  } catch (...) {
    return default_value;
  }
}

static inline long parse_long(std::string_view str, long default_value) {
  try {
    return std::stol(std::string(str));
  } catch (...) {
    return default_value;
  }
}

}  // namespace util
}  // namespace alpaca

#endif  // INCLUDE_UTIL_HPP_
