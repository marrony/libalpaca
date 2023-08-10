// Copyright (C) 2023 Marrony Neris

#ifndef INCLUDE_PARSER_HPP_
#define INCLUDE_PARSER_HPP_

#include <string>
#include <cstdio>
#include <stdexcept>
#include <optional>
#include <string_view>

#include <errors.hpp>
#include <c++util.hpp>
#include <util.hpp>

namespace alpaca {
namespace parser {

template<typename T>
struct conversor {
  auto conv(std::string_view) -> result<T, alpaca_error>;
};

template<>
struct conversor<bool> {
  auto conv(std::string_view v) -> result<bool, alpaca_error> {
    if (util::equals_insentive(v, "true")) {
      return true;
    }

    if (util::equals_insentive(v, "false")) {
      return false;
    }

    return custom_error("not valid bool");
  }
};

template<>
struct conversor<int> {
  auto conv(std::string_view v) -> result<int, alpaca_error> {
    char* end = nullptr;
    long value = std::strtol(v.data(), &end, 10);

    if (v.data() != end)
     return static_cast<int>(value);

    return custom_error("not valid int");
  }
};

template<>
struct conversor<float> {
  auto conv(std::string_view v) -> result<float, alpaca_error> {
    char* end = nullptr;
    float value = std::strtof(v.data(), &end);

    if (v.data() != end)
      return value;

    return custom_error("not valid float");
  }
};

template<>
struct conversor<std::string_view> {
  auto conv(std::string_view v) -> result<std::string_view, alpaca_error> {
    return v;
  }
};

template<typename T>
struct field {
  const char* name;

  auto get(const arguments_t& args) const -> result<T, alpaca_error> {
    // todo(marrony): key is a std::string
    if (auto ite = args.find(name); ite != args.end()) {
      if (auto value = conversor<T>{}.conv(ite->second); !value.is_error())
        return value.get();

      return custom_error(std::string{"Invalid '"} + name + "' field");
    } else {
      return custom_error(std::string{"Field '"} + name + "' not found");
    }
  }
};

struct parser_t {
  template<typename T, typename ...C>
  static result<T, alpaca_error> parse(const arguments_t& args, const C&... cs) {
    return visit(
      [](auto&&... v) {
        return T(v...);
      },
      cs.get(args)...
    );
  }
};

}  // namespace parser
}  // namespace alpaca

#endif  // INCLUDE_PARSER_HPP_
