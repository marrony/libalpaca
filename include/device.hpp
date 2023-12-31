// Copyright (C) 2023 Marrony Neris

#ifndef INCLUDE_DEVICE_HPP_
#define INCLUDE_DEVICE_HPP_

#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string_view>

#include <resource.hpp>
#include <util.hpp>
#include <json.hpp>
#include <parser.hpp>
#include <fields.hpp>

namespace alpaca {

struct deviceinfo_t {
  std::string name;
  std::string device_type;
  int device_number;
  std::string unique_id;
};

using check_t = result<void, alpaca_error>;

class device {
 protected:
  int device_number = -1;
  bool is_connected = false;

  [[nodiscard]]
  inline auto check_connected() const -> check_t {
    if (!is_connected) return not_connected();

    return {};
  }

  [[nodiscard]]
  inline auto check_flag(return_t<bool>&& flag) const -> check_t {
    return flag.flat_map([](bool flag) -> check_t {
      if (!flag) return not_implemented();

      return {};
    });
  }

  [[nodiscard]]
  inline auto check_op(bool completed) const -> check_t {
    if (!completed) return invalid_operation();

    return {};
  }

  [[nodiscard]]
  inline auto check_init(bool initialized) const -> check_t {
    if (!initialized) return value_not_set();

    return {};
  }

  [[nodiscard]]
  inline auto check_value(bool correct) const -> check_t {
    if (!correct) return invalid_value();

    return {};
  }

  [[nodiscard]]
  inline auto check_set(bool set) const -> check_t {
    if (!set) return value_not_set();

    return {};
  }

 public:
  virtual ~device()
  { }

  virtual void set_device_number(int device_number) {
    this->device_number = device_number;
  }

  virtual return_t<void> put_connected(bool connected) {
    if (is_connected && connected) return {};
    if (!is_connected && !connected) return {};
    is_connected = connected;
    return {};
  }

  virtual return_t<bool> get_connected() const {
    return is_connected;
  }

  virtual return_t<void> put_action() {
    return {};
  }

  virtual return_t<void> put_commandblind() {
    return {};
  }

  virtual return_t<void> put_commandbool() {
    return {};
  }

  virtual return_t<void> put_commandstring() {
    return {};
  }

  virtual return_t<std::string> get_description() const = 0;
  virtual return_t<std::string> get_driverinfo() const = 0;
  virtual return_t<std::string> get_driverversion() const = 0;
  virtual return_t<int> get_interfaceversion() const = 0;
  virtual return_t<std::string> get_name() const = 0;
  virtual return_t<std::vector<std::string>> get_supportedactions() const = 0;
  virtual return_t<deviceinfo_t> get_deviceinfo() const = 0;
};

template<typename T>
requires std::is_base_of_v<device, T>
class device_resource : public alpaca_resource {
 protected:
  std::string device_type;
  std::vector<T*> devices;

  using get_fn = std::function<return_t<json_value>(const T*, const arguments_t&)>;
  using put_fn = return_t<void>(*)(T*, const arguments_t&);

  std::map<std::string_view, get_fn> get_operations;
  std::map<std::string_view, put_fn> put_operations;

  inline void define_get(std::string_view op, get_fn get) {
    get_operations[op] = get;  }

  inline void define_put(std::string_view op, put_fn put) {
    put_operations[op] = put;
  }

  inline void define_ops(std::string_view op, get_fn get, put_fn put) {
    get_operations[op] = get;
    put_operations[op] = put;
  }

 protected:
  virtual return_t<json_value> handle_get(
    const httpserver::http_request& req,
    const arguments_t& args) {

    if (req.get_path_piece(2) != device_type) {
      return http_error(404, "not found");
    }

    int device_id = util::parse_int(req.get_path_piece(3), -1);

    if (device_id < 0 || static_cast<std::size_t>(device_id) > devices.size()) {
      return http_error(404, "not found");
    }

    T* device = devices[device_id];

    auto operation = req.get_path_piece(4);

    if (req.get_method() == "GET") {
      auto op = get_operations.find(operation);
      if (op != get_operations.end()) {
        return op->second(device, args);
      } else {
        return http_error(404, "not found");
      }
    }

    if (req.get_method() == "PUT") {
      auto op = put_operations.find(operation);
      if (op != put_operations.end()) {
        return op->second(device, args).map([]() {
          return static_cast<json_value>( nullptr );
        });
      } else {
        return http_error(404, "not found");
      }
    }

    return http_error(400, "bad request");
  }

 public:
  device_resource(const std::string& device_type)
  : device_type(device_type), devices() {
    define_put("action", [](T* device, const arguments_t&) {
      return device->put_action();
    });

    define_put("commandblind", [](T* device, const arguments_t&) {
      return device->put_commandblind();
    });

    define_put("commandbool", [](T* device, const arguments_t&) {
      return device->put_commandbool();
    });

    define_put("commandstring", [](T* device, const arguments_t&) {
      return device->put_commandstring();
    });

    define_ops(
      "connected",
      [](const T* dev, const arguments_t&) -> return_t<json_value> {
        return dev->get_connected();
      },
      [](T* dev, const arguments_t& args) {
        return parser::parser_t::parse<bool>(args, fields::connected_f)
          .map([dev](bool connected) {
            dev->put_connected(connected);
          });
      });

    define_get("description", [](const T* dev, const arguments_t&) -> return_t<json_value> {
      return dev->get_description();
    });
    define_get("driverinfo", [](const T* dev, const arguments_t&) -> return_t<json_value> {
      return dev->get_driverinfo();
    });
    define_get("driverversion", [](const T* dev, const arguments_t&) -> return_t<json_value> {
      return dev->get_driverversion();
    });
    define_get("interfaceversion", [](const T* dev, const arguments_t&) -> return_t<json_value> {
      return dev->get_interfaceversion();
    });
    define_get("name", [](const T* dev, const arguments_t&) -> return_t<json_value> {
      return dev->get_name();
    });

    define_get("supportedactions", [](const T* dev, const arguments_t&) -> return_t<json_value> {
      return dev->get_supportedactions().map([](const auto& supportedactions) {
        json_array actions;
        std::copy(
          std::cbegin(supportedactions),
          std::cend(supportedactions),
          std::begin(actions)
        );
        return actions;
      });
    });
  }

  void add_device(T* device) {
    int device_number = static_cast<int>(devices.size());
    device->set_device_number(device_number);
    devices.push_back(device);
  }
};

}  // namespace alpaca

#endif  // INCLUDE_DEVICE_HPP_
