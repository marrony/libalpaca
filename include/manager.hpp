// Copyright (C) 2023 Marrony Neris

#ifndef INCLUDE_MANAGER_HPP_
#define INCLUDE_MANAGER_HPP_

#include <algorithm>
#include <sstream>

#include <httpserver.hpp>
#include <create_webserver.hpp>

#include <resource.hpp>
#include <json.hpp>
#include <device.hpp>
#include <telescope.hpp>

namespace alpaca {

class device_manager {
  std::vector<device*> devices;

  class apiversions_resource : public alpaca_resource {
   public:
    apiversions_resource() { }

    virtual json_value handle_get(
      const httpserver::http_request& req,
      const arguments_t& args) {
      json_array response = { 1 };
      return response;
    }
  };

  class description_resource : public alpaca_resource {
   public:
    description_resource() { }

    virtual json_value handle_get(
      const httpserver::http_request& req,
      const arguments_t& args) {

      return (json_object) {
        {"ServerName", "Alpaca Telescope Server"},
        {"Manufacturer", "Marrony Neris"},
        {"ManufacturerVersion", "0.0.1"},
        {"Location", "US"}
      };
    }
  };

  class configureddevices_resource : public alpaca_resource {
    device_manager* manager;
   public:
    configureddevices_resource(device_manager* manager)
    : manager(manager) { }

    virtual json_value handle_get(
      const httpserver::http_request& req,
      const arguments_t& args) {
      json_array response;

      std::transform(
        std::cbegin(manager->devices),
        std::cend(manager->devices),
        std::back_inserter(response),
        [](device* device) {
          deviceinfo_t info = device->get_deviceinfo();

          return (json_object) {
            {"DeviceName", info.name},
            {"DeviceType", info.device_type},
            {"DeviceNumber", info.device_number},
            {"UniqueID", info.unique_id},
          };
        });

      return response;
    }
  };

  apiversions_resource apiversions;
  description_resource description;
  configureddevices_resource configureddevices;
  telescope_resource telescopes;
  telescope_setup_resource telescope_setup;

  void register_endpoint(httpserver::webserver* ws) {
    ws->register_resource("/management/apiversions", &apiversions);
    ws->register_resource("/management/v1/description", &description);
    ws->register_resource("/management/v1/configureddevices", &configureddevices);

    ws->register_resource("/api/v1/telescope", &telescopes, true);
    ws->register_resource("/setup/v1/telescope", &telescope_setup, true);
  }

 public:
  device_manager()
  : apiversions()
  , description()
  , configureddevices(this)
  , telescopes()
  , telescope_setup(&telescopes)
  { }

  void add_telescope(telescope* telescope) {
    devices.push_back(telescope);
    telescopes.add_device(telescope);
  }

  int run(int port) {
    httpserver::webserver ws = httpserver::create_webserver(port)
      .no_post_process();

    register_endpoint(&ws);

    ws.start(true);

    return 0;
  }
};

}  // namespace alpaca

#endif  // INCLUDE_MANAGER_HPP_