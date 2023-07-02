// Copyright (C) 2023 Marrony Neris

#ifndef INCLUDE_RESOURCE_HPP_
#define INCLUDE_RESOURCE_HPP_

#include <atomic>
#include <iostream>
#include <sstream>

#include <httpserver.hpp>

#include <json.hpp>
#include <types.hpp>
#include <util.hpp>
#include <errors.hpp>

namespace alpaca {

class alpaca_resource : public httpserver::http_resource {

 protected:
  virtual json_value handle_get(
    const httpserver::http_request& req,
    const arguments_t& args) {
    return nullptr;
  }

  virtual void handle_put(
    const httpserver::http_request& req,
    const arguments_t& args) { }

  std::shared_ptr<httpserver::http_response> not_found() {
    return std::make_shared<httpserver::string_response>("Not Found", 404);
  }

  std::shared_ptr<httpserver::http_response> method_not_allowed() {
    return std::make_shared<httpserver::string_response>("Method Not Allowed", 405);
  }

  std::shared_ptr<httpserver::http_response> bad_request(const std::string& msg) {
    return std::make_shared<httpserver::string_response>(msg, 400);
  }

  std::shared_ptr<httpserver::http_response> ok(const json_value& response) {
    std::ostringstream os;

    os << response;

    return std::make_shared<httpserver::string_response>(os.str(), 200, "application/json");
  }

 public:
  virtual std::shared_ptr<httpserver::http_response> render(const httpserver::http_request& req) override {
    comparator_t comp = {
      req.get_method() == "GET"
      ? util::compare_less_insensitive
      : util::compare_less_sensitive
    };
    arguments_t args(comp);

    std::string_view to_parse =
      req.get_method() == "PUT"
      ? req.get_content()
      : req.get_querystring().substr(1);

    for (auto& token : util::split(to_parse, "&")) {
      auto a = util::split(token, "=");
      if (a.size() > 0) {
        std::string key{ a[0] };

        if (a.size() == 2)
          args[key] = a[1];

        httpserver::http::http_unescape(&args[key]);
      }
    }

    std::uint32_t client_transaction_id = 0;
    if (auto ite = args.find("ClientTransactionID"); ite != args.end()) {
      if (std::sscanf(ite->second.c_str(), "%u", &client_transaction_id) != 1)
        return bad_request("Invalid 'ClientTransactionID'");
    }

    std::uint32_t client_id = 0;
    if (auto ite = args.find("ClientID"); ite != args.end()) {
      if (std::sscanf(ite->second.c_str(), "%u", &client_id) != 1)
        return bad_request("Invalid 'ClientID'");
    }

    static std::atomic<int> server_transaction_id;

    json_object js_out = {
      {"ClientID", static_cast<json_int>(client_id)},
      {"ErrorNumber", 0},
      {"ErrorMessage", std::string("")},
      {"ClientTransactionID", static_cast<json_int>(client_transaction_id)},
      {"ServerTransactionID", ++server_transaction_id}
    };

    try {
      if (req.get_method() == "GET") {
        js_out["Value"] = handle_get(req, args);

        std::cout << req.get_method() << " " << req.get_path() << "?" << to_parse;
        std::cout << " => " << js_out["Value"] << std::endl;
      }

      if (req.get_method() == "PUT") {
        std::cout << req.get_method() << " " << req.get_path() << "?" << to_parse << std::endl;

        handle_put(req, args);
      }

      return ok(js_out);
    } catch (error::not_found& ex) {
      return not_found();
    } catch (error::alpaca_error& ex) {
      js_out["ErrorNumber"] = ex.error_number;
      js_out["ErrorMessage"] = ex.error_message;
      return ok(js_out);
    } catch (std::exception& ex) {
      std::cout << ex.what() << std::endl;
      return bad_request(ex.what());
    }
  }
};

}  // namespace alpaca

#endif  // INCLUDE_RESOURCE_HPP_
