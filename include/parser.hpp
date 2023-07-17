// Copyright (C) 2023 Marrony Neris

#ifndef INCLUDE_PARSER_HPP_
#define INCLUDE_PARSER_HPP_

#include <string>
#include <cstdio>
#include <stdexcept>
#include <string_view>
#include <util.hpp>
#include <types.hpp>

namespace alpaca {
namespace parser {

template<typename T>
struct conversor {
  static T conv(std::string_view);
};

template<>
struct conversor<bool> {
  static bool conv(std::string_view v) {
    if (util::equals_insentive(v, "true")) {
      return true;
    }

    if (util::equals_insentive(v, "false")) {
      return false;
    }

    throw std::invalid_argument(std::string(v));
  }
};

template<>
struct conversor<int> {
  static int conv(std::string_view v) {
    int value;
    if (std::sscanf(v.data(), "%d", &value) == 1) {
      return value;
    }
    throw std::invalid_argument(std::string(v));
  }
};

template<>
struct conversor<float> {
  static float conv(std::string_view v) {
    float value;
    if (std::sscanf(v.data(), "%f", &value) == 1) {
      return value;
    }
    throw std::invalid_argument(std::string(v));
  }
};

template<>
struct conversor<std::string> {
  static std::string conv(std::string_view v) {
    return std::string(v);
  }
};

template<typename T>
struct field {
  const char* name;

  T get(const arguments_t& args) const {
    if (auto ite = args.find(name); ite != args.end()) {
      try {
        return conversor<T>().conv(ite->second);
      } catch(...) {
        throw std::invalid_argument(std::string("Invalid '") + name + "' field");
      }
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
