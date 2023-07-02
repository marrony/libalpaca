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
#include <types.hpp>

namespace alpaca {

struct deviceinfo_t {
  std::string name;
  std::string device_type;
  int device_number;
  std::string unique_id;
};

class device {
 protected:
  int device_number;
 public:
  virtual ~device() {}

  virtual void set_device_number(int device_number) {
    this->device_number = device_number;
  }

  virtual void put_action() {}
  virtual void put_commandblind() {}
  virtual void put_commandbool() {}
  virtual void put_commandstring() {}
  virtual void put_connected(bool) = 0;
  virtual bool get_connected() const = 0;
  virtual std::string get_description() const = 0;
  virtual std::string get_driverinfo() const = 0;
  virtual std::string get_driverversion() const = 0;
  virtual int get_interfaceversion() const = 0;
  virtual std::string get_name() const = 0;
  virtual const std::vector<std::string>& get_supportedactions() const = 0;
  virtual deviceinfo_t get_deviceinfo() const = 0;
};

// template<class T, class U>
// concept Derived = std::is_base_of_v<U, T>;

template<typename T>
class device_resource : public alpaca_resource {
  static_assert(std::is_base_of_v<device, T>, "T must be derived from device");

 protected:
  std::string device_type;
  std::vector<T*> devices;

  using get_fn = std::function<json_value(const T*, const arguments_t&)>;
  using put_fn = std::function<void(T*, const arguments_t&)>;

  std::map<std::string, get_fn> get_operations;
  std::map<std::string, put_fn> put_operations;

  void define_get(const char* op, get_fn get) {
    get_operations[op] = get;
  }

  void define_put(const char* op, put_fn put) {
    put_operations[op] = put;
  }

  void define_ops(const char* op, get_fn get, put_fn put) {
    get_operations[op] = get;
    put_operations[op] = put;
  }

 protected:
  virtual json_value handle_get(
    const httpserver::http_request& req,
    const arguments_t& args) {

    if (req.get_path_piece(2) != device_type) {
      throw error::not_found();
    }

    int device_id = util::parse_int(req.get_path_piece(3), -1);

    if (device_id < 0 || static_cast<std::size_t>(device_id) > devices.size()) {
      throw error::not_found();
    }

    T* device = devices[device_id];

    auto operation = req.get_path_piece(4);

    auto op = get_operations.find(operation);
    if (op != get_operations.end()) {
      return op->second(device, args);
    } else {
      throw error::not_found();
    }
  }

  virtual void handle_put(
    const httpserver::http_request& req,
    const arguments_t& args) {

    if (req.get_path_piece(2) != device_type) {
      throw error::not_found();
    }

    int device_id = util::parse_int(req.get_path_piece(3), -1);

    if (device_id < 0 || static_cast<std::size_t>(device_id) > devices.size()) {
      throw error::not_found();
    }

    T* device = devices[device_id];

    auto operation = req.get_path_piece(4);

    auto op = put_operations.find(operation);
    if (op != put_operations.end()) {
      op->second(device, args);
    } else {
      throw error::not_found();
    }
  }

 public:
  device_resource(const std::string& device_type)
  : device_type(device_type), devices() {
    define_put("action", [](T* device, const arguments_t& args) {
      device->put_action();
    });

    define_put("commandblind", [](T* device, const arguments_t& args) {
      device->put_commandblind();
    });

    define_put("commandbool", [](T* device, const arguments_t& args) {
      device->put_commandbool();
    });

    define_put("commandstring", [](T* device, const arguments_t& args) {
      device->put_commandstring();
    });

    define_ops(
      "connected",
      [](const T* dev, const arguments_t& args) {
        return dev->get_connected();
      },
      [](T* dev, const arguments_t& args) {
        dev->put_connected(parser::parser_t::parse<bool>(args, fields::connected_f));
      });

    define_get("description", [](const T* dev, const arguments_t& args) {
      return dev->get_description();
    });
    define_get("driverinfo", [](const T* dev, const arguments_t& args) {
      return dev->get_driverinfo();
    });
    define_get("driverversion", [](const T* dev, const arguments_t& args) {
      return dev->get_driverversion();
    });
    define_get("interfaceversion", [](const T* dev, const arguments_t& args) {
      return dev->get_interfaceversion();
    });
    define_get("name", [](const T* dev, const arguments_t& args) {
      return dev->get_name();
    });

    define_get("supportedactions", [](const T* dev, const arguments_t&) {
      auto supportedactions = dev->get_supportedactions();
      json_array actions;
      std::copy(
        std::cbegin(supportedactions),
        std::cend(supportedactions),
        std::begin(actions)
      );
      return actions;
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
