// Copyright (C) 2023 Marrony Neris

#ifndef INCLUDE_RESOURCE_HPP_
#define INCLUDE_RESOURCE_HPP_

#include <atomic>
#include <iostream>
#include <sstream>

#include <httpserver.hpp>

#include <c++util.hpp>
#include <json.hpp>
#include <util.hpp>
#include <errors.hpp>

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

enum class http_error_kind {
  not_found,
  bad_request
};

using return_t = ret_t<
  json_value,
  http_error_kind
>;

class alpaca_resource : public httpserver::http_resource {

 protected:
  virtual return_t handle_get(
    const httpserver::http_request& req,
    const arguments_t& args) = 0;

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
      auto handle_return = [&](return_t&& ret) {
        std::cout << req.get_method() << " " << req.get_path() << "?" << to_parse;
        return std::visit(
          overloaded {
            [&](const json_value& value) {
              if (value != nullptr)
                std::cout << " => " << value << std::endl;
              else
                std::cout << std::endl;

              js_out["Value"] = value;
              return ok(js_out);
            },
            [=](http_error_kind error) {
              switch (error) {
                case http_error_kind::not_found:
                  return not_found();
                case http_error_kind::bad_request:
                  return bad_request("invalid method");
              }
            }
          },
          ret
        );
      };

      return handle_return(handle_get(req, args));
    } catch (error::alpaca_error& ex) {
      js_out["ErrorNumber"] = ex.error_number;
      js_out["ErrorMessage"] = ex.error_message;
      return ok(js_out);
    } catch (...) {
      std::cout << "unknown exception" << std::endl;
      return bad_request("unknown exception");
    }
  }
};

}  // namespace alpaca

#endif  // INCLUDE_RESOURCE_HPP_
