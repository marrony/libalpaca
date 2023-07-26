// Copyright (C) 2023 Marrony Neris

#ifndef INCLUDE_PARSER_HPP_
#define INCLUDE_PARSER_HPP_

#include <string>
#include <cstdio>
#include <stdexcept>
#include <optional>
#include <string_view>
#include <util.hpp>

namespace alpaca {
namespace parser {

template<typename T>
struct conversor {
  auto conv(std::string_view) -> std::optional<T>;
};

template<>
struct conversor<bool> {
  auto conv(std::string_view v) -> std::optional<bool> {
    if (util::equals_insentive(v, "true")) {
      return true;
    }

    if (util::equals_insentive(v, "false")) {
      return false;
    }

    return std::nullopt;
  }
};

template<>
struct conversor<int> {
  auto conv(std::string_view v) -> std::optional<int> {
    char* end = nullptr;
    long value = std::strtol(v.data(), &end, 10);

    if (v.data() != end)
     return static_cast<int>(value);

    return std::nullopt;
  }
};

template<>
struct conversor<float> {
  auto conv(std::string_view v) -> std::optional<float> {
    char* end = nullptr;
    float value = std::strtof(v.data(), &end);

    if (v.data() != end)
      return value;

    return std::nullopt;
  }
};

template<>
struct conversor<std::string_view> {
  auto conv(std::string_view v) -> std::optional<std::string_view> {
    return v;
  }
};

template<typename T>
struct field {
  const char* name;

  T get(const arguments_t& args) const {
    // todo(marrony): key is a std::string
    if (auto ite = args.find(name); ite != args.end()) {
      if (auto value = conversor<T>{}.conv(ite->second))
        return *value;

      throw std::invalid_argument(std::string("Invalid '") + name + "' field");
    } else {
      throw std::invalid_argument(std::string("Field '") + name + "' not found");
    }
  }
};

struct parser_t {
  template<typename T, typename ...C>
  static T parse(const arguments_t& args, const C&... cs) {
    return T(cs.get(args)...);
  }
};

}  // namespace parser
}  // namespace alpaca

#endif  // INCLUDE_PARSER_HPP_
