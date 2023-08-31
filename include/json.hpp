// Copyright (C) 2023 Marrony Neris

#ifndef INCLUDE_JSON_HPP_
#define INCLUDE_JSON_HPP_

#include <string>
#include <vector>
#include <map>
#include <iomanip>
#include <rva/variant.hpp>

namespace alpaca {

using json_value = rva::variant<
  std::nullptr_t,
  bool,
  long,
  float,
  std::string,
  std::vector<rva::self_t>,
  std::map<std::string, rva::self_t>
>;

using json_int = long;
using json_float = float;
using json_bool = bool;
using json_string = std::string;
using json_array = std::vector<json_value>;
using json_object = std::map<std::string, json_value>;

}  // namespace alpaca

static inline std::ostream& operator<<(std::ostream& os, const alpaca::json_value& js) {
  switch (js.index()) {
    case 0:
      os << "null";
      break;
    case 1:
      os << std::boolalpha << std::get<alpaca::json_bool>(js);
      break;
    case 2:
      os << std::get<alpaca::json_int>(js);
      break;
    case 3:
      os << std::get<alpaca::json_float>(js);
      break;
    case 4:
      os << std::quoted(std::get<alpaca::json_string>(js));
      break;
    case 5: {
      auto values = std::get<alpaca::json_array>(js);

      os << "[";
      for (auto i = values.begin(); i != values.end(); i++) {
        os << *i;
        if (i+1 != values.end())
          os << ",";
      }
      os << "]";
      break;
    }
    case 6: {
      auto pairs = std::get<alpaca::json_object>(js);
      os << "{";
      auto kv = pairs.begin();
      while (kv != pairs.end()) {
        os << std::quoted(kv->first) << ":" << kv->second;

        kv++;

        if (kv != pairs.end())
          os << ",";
      }
      os << "}";
      break;
    }
  }

  return os;
}

#endif  // INCLUDE_JSON_HPP_
