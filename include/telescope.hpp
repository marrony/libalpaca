// Copyright (C) 2023 Marrony Neris

#ifndef INCLUDE_TELESCOPE_HPP_
#define INCLUDE_TELESCOPE_HPP_

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <stdexcept>

#include <device.hpp>
#include <parser.hpp>
#include <fields.hpp>
#include <types.hpp>
#include <json.hpp>

namespace alpaca {

struct telescope_flags_t {
  enum {
    NONE                         = 0x00000,
    CAN_FIND_HOME                = 0x00001,
    CAN_PARK                     = 0x00002,
    CAN_PULSE_GUIDE              = 0x00004,
    CAN_SET_DECLINATION_RATE     = 0x00008,
    CAN_SET_GUIDE_RATES          = 0x00010,
    CAN_SET_PARK                 = 0x00020,
    CAN_SET_PIER_SIDE            = 0x00040,
    CAN_SET_RIGHT_ASCENSION_RATE = 0x00080,
    CAN_SET_TRACKING             = 0x00100,
    CAN_SLEW                     = 0x00200,
    CAN_SLEW_ALTAZ               = 0x00400,
    CAN_SLEW_ALTAZ_ASYNC         = 0x00800,
    CAN_SLEW_ASYNC               = 0x01000,
    CAN_SYNC                     = 0x02000,
    CAN_SYNC_ALTAZ               = 0x04000,
    CAN_UNPARK                   = 0x08000,
    CAN_MOVE_AXIS_0              = 0x10000,
    CAN_MOVE_AXIS_1              = 0x20000,
    CAN_MOVE_AXIS_2              = 0x40000,
  };
};

struct telescopeinfo_t {
  std::string description;
  std::string driverinfo;
  std::string driverversion;
  int interfaceversion;
  std::string name;
  alignment_mode_t alignmentmode;
  float aperturearea;
  float aperturediameter;
  float focallength;
  equatorial_system_t equatorialsystem;
  std::vector<axis_rate_t> axisrates;
  std::vector<driver_rate_t> trackingrates;
  std::uint32_t flags;
};

struct altazm_t {
  const float altitude;
  const float azimuth;

  altazm_t(float altitude, float azimuth)
  : altitude(altitude), azimuth(azimuth) { }

  static altazm_t parse(const arguments_t& args) {
    return parser::parser_t::parse<altazm_t>(args, fields::altitude_f, fields::azimuth_f);
  }
};

struct coord_t {
  const float rightascension;
  const float declination;

  coord_t(float rightascension, float declination)
  : rightascension(rightascension), declination(declination) { }

  static coord_t parse(const arguments_t& args) {
    return parser::parser_t::parse<coord_t>(
      args,
      fields::rightascension_f,
      fields::declination_f);
  }
};

struct pulse_t {
  const int direction;
  const int duration;

  pulse_t(int direction, int duration)
  : direction(direction), duration(duration) { }

  static pulse_t parse(const arguments_t& args) {
    return parser::parser_t::parse<pulse_t>(
      args,
      fields::direction_f,
      fields::duration_f);
  }
};

struct move_t {
  const int axis;
  const float rate;  // deg/sec

  move_t(int axis, float rate)
  : axis(axis), rate(rate) { }

  static move_t parse(const arguments_t& args) {
    return parser::parser_t::parse<move_t>(args, fields::axis_f, fields::rate_f);
  }
};

class telescope : public device {
 protected:
  telescopeinfo_t telescopeinfo;
  bool is_connected;

  inline void check_connected() const {
    if (!is_connected) throw error::not_connected();
  }

  inline void check_parked() const {
    if (get_atpark()) throw error::parked();
  }

  inline void check_flag(bool flag) const {
    if (!flag) throw error::not_implemented();
  }

  inline void not_implemented() const {
    check_flag(false);
  }

  inline void check_op(bool completed) const  {
    if (!completed) throw error::invalid_operation();
  }

  inline void check_init(bool initialized) const {
    if (!initialized) throw error::value_not_set();
  }

  inline void check_value(bool correct) const {
    if (!correct) throw error::invalid_value();
  }

  inline void check_set(bool set) const {
    if (!set) throw error::value_not_set();
  }

 public:
  explicit telescope(const telescopeinfo_t& telescopeinfo)
  : telescopeinfo(telescopeinfo)
  , is_connected(false)
  { }

  // read-only properties
  virtual float get_altitude() const = 0;
  virtual float get_azimuth() const = 0;
  virtual float get_declination() const = 0;
  virtual float get_rightascension() const = 0;
  virtual bool get_athome() const = 0;
  virtual bool get_atpark() const = 0;
  virtual bool get_ispulseguiding() const = 0;
  virtual bool get_slewing() const = 0;
  virtual float get_siderealtime() const = 0;
  virtual destination_side_of_pier_t get_destinationsideofpier(
    float rightascension, float declination) const = 0;

  // constants
  virtual std::string get_description() const { return telescopeinfo.description; }
  virtual std::string get_driverinfo() const { return telescopeinfo.driverinfo; }
  virtual std::string get_driverversion() const { return telescopeinfo.driverversion; }
  virtual int get_interfaceversion() const { return telescopeinfo.interfaceversion; }
  virtual std::string get_name() const { return telescopeinfo.name; }
  virtual const std::vector<std::string>& get_supportedactions() const {
    static std::vector<std::string> actions;
    return actions;
  }
  virtual deviceinfo_t get_deviceinfo() const = 0;
  virtual alignment_mode_t get_alignmentmode() const { return telescopeinfo.alignmentmode; }
  virtual float get_aperturearea() const { return telescopeinfo.aperturearea; }
  virtual float get_aperturediameter() const { return telescopeinfo.aperturediameter; }
  virtual float get_focallength() const { return telescopeinfo.focallength; }
  virtual equatorial_system_t get_equatorialsystem() const {
    return telescopeinfo.equatorialsystem;
  }
  virtual const std::vector<axis_rate_t>& get_axisrates(int axis) const {
    return telescopeinfo.axisrates;
  }
  virtual const std::vector<driver_rate_t>& get_trackingrates() const {
    return telescopeinfo.trackingrates;
  }

  // read-wrie properties
  virtual float get_declinationrate() const {
    check_connected();
    return 0;
  }

  virtual void put_declinationrate(float) {
    check_connected();
    check_flag(get_cansetdeclinationrate());
  }

  virtual float get_rightascensionrate() const {
    check_connected();
    return 0;
  }

  virtual void put_rightascensionrate(float) {
    check_connected();
    check_flag(get_cansetrightascensionrate());
  }

  virtual bool get_doesrefraction() const {
    check_connected();
    not_implemented();
    return false;
  }
  virtual void put_doesrefraction(bool) {
    check_connected();
    not_implemented();
  }

  virtual float get_guideratedeclination() const = 0;
  virtual void put_guideratedeclination(float) = 0;

  virtual float get_guideraterightascension() const = 0;
  virtual void put_guideraterightascension(float) = 0;

  virtual int get_sideofpier() const {
    check_connected();
    return 0;
  }

  virtual void put_sideofpier(int) = 0;

  virtual float get_siteelevation() const {
    check_connected();
    not_implemented();
    return 0.0f;
  }

  virtual void put_siteelevation(float elevation) {
    check_connected();
    check_value(elevation >= -300.0f && elevation <= 10000.0f);
    not_implemented();
  }

  virtual float get_sitelatitude() const = 0;
  virtual void put_sitelatitude(float) = 0;

  virtual float get_sitelongitude() const = 0;
  virtual void put_sitelongitude(float) = 0;

  virtual int get_slewsettletime() const = 0;
  virtual void put_slewsettletime(int) = 0;

  virtual float get_targetdeclination() const = 0;
  virtual void put_targetdeclination(float) = 0;

  virtual float get_targetrightascension() const = 0;
  virtual void put_targetrightascension(float) = 0;

  virtual bool get_tracking() const = 0;
  virtual void put_tracking(bool) = 0;

  virtual driver_rate_t get_trackingrate() const = 0;
  virtual void put_trackingrate(driver_rate_t) = 0;

  virtual std::string get_utcdate() const = 0;
  virtual void put_utcdate(const std::string& utc) = 0;

  // flags
  virtual bool get_canfindhome() const {
    return (telescopeinfo.flags & telescope_flags_t::CAN_FIND_HOME);
  }
  virtual bool get_canmoveaxis(int axis) const {
    return telescopeinfo.flags & (telescope_flags_t::CAN_MOVE_AXIS_0 << axis);
  }
  virtual bool get_canpark() const {
    return telescopeinfo.flags & telescope_flags_t::CAN_PARK;
  }
  virtual bool get_canpulseguide() const {
    return telescopeinfo.flags & telescope_flags_t::CAN_PULSE_GUIDE;
  }
  virtual bool get_cansetdeclinationrate() const {
    return telescopeinfo.flags & telescope_flags_t::CAN_SET_DECLINATION_RATE;
  }
  virtual bool get_cansetguiderates() const {
    return telescopeinfo.flags & telescope_flags_t::CAN_SET_GUIDE_RATES;
  }
  virtual bool get_cansetpark() const {
    return telescopeinfo.flags & telescope_flags_t::CAN_SET_PARK;
  }
  virtual bool get_cansetpierside() const {
    return telescopeinfo.flags & telescope_flags_t::CAN_SET_PIER_SIDE;
  }
  virtual bool get_cansetrightascensionrate() const {
    return telescopeinfo.flags & telescope_flags_t::CAN_SET_RIGHT_ASCENSION_RATE;
  }
  virtual bool get_cansettracking() const {
    return telescopeinfo.flags & telescope_flags_t::CAN_SET_TRACKING;
  }
  virtual bool get_canslew() const {
    return telescopeinfo.flags & telescope_flags_t::CAN_SLEW;
  }
  virtual bool get_canslewaltaz() const {
    return telescopeinfo.flags & telescope_flags_t::CAN_SLEW_ALTAZ;
  }
  virtual bool get_canslewaltazasync() const {
    return telescopeinfo.flags & telescope_flags_t::CAN_SLEW_ALTAZ_ASYNC;
  }
  virtual bool get_canslewasync() const {
    return telescopeinfo.flags & telescope_flags_t::CAN_SLEW_ASYNC;
  }
  virtual bool get_cansync() const {
    return telescopeinfo.flags & telescope_flags_t::CAN_SYNC;
  }
  virtual bool get_cansyncaltaz() const {
    return telescopeinfo.flags & telescope_flags_t::CAN_SYNC_ALTAZ;
  }
  virtual bool get_canunpark() const {
    return telescopeinfo.flags & telescope_flags_t::CAN_UNPARK;
  }

  // operations
  virtual void abortslew() = 0;
  virtual void findhome() {
    check_connected();
    check_flag(get_canfindhome());
  }
  virtual void moveaxis(int axis, float rate) {
    check_connected();
    check_flag(get_canmoveaxis(axis));
    check_value(rate > -9.0f && rate < +9.0f);
  }
  virtual void park() {
    check_connected();
    check_flag(get_canpark());
  }
  virtual void pulseguide(int direction, int duration) = 0;
  virtual void setpark() = 0;
  virtual void slewtoaltaz(float altitude, float azimuth) = 0;
  virtual void slewtoaltazasync(float altitude, float azimuth) = 0;
  virtual void slewtocoordinates(float rightascension, float declination) = 0;
  virtual void slewtocoordinatesasync(float rightascension, float declination) = 0;
  virtual void slewtotarget() = 0;
  virtual void slewtotargetasync() = 0;
  virtual void synctoaltaz(float altitude, float azimuth) = 0;
  virtual void synctocoordinates(float rightascension, float declination) = 0;
  virtual void synctotarget() = 0;
  virtual void unpark() = 0;
};

class telescope_resource : public device_resource<telescope> {
 public:
  telescope_resource()
  : device_resource("telescope") {
    // read-only properties
    define_get("altitude", [](const telescope* tel, const arguments_t& args) {
      return tel->get_altitude();
    });
    define_get("azimuth", [](const telescope* tel, const arguments_t& args) {
      return tel->get_azimuth();
    });
    define_get("declination", [](const telescope* tel, const arguments_t& args) {
      return tel->get_declination();
    });
    define_get("rightascension", [](const telescope* tel, const arguments_t& args) {
      return tel->get_rightascension();
    });
    define_get("athome", [](const telescope* tel, const arguments_t& args) {
      return tel->get_athome();
    });
    define_get("atpark", [](const telescope* tel, const arguments_t& args) {
      return tel->get_atpark();
    });
    define_get("ispulseguiding", [](const telescope* tel, const arguments_t& args) {
      return tel->get_ispulseguiding();
    });
    define_get("slewing", [](const telescope* tel, const arguments_t& args) {
      return tel->get_slewing();
    });
    define_get("siderealtime", [](const telescope* tel, const arguments_t& args) {
      return tel->get_siderealtime();
    });
    define_get("destinationsideofpier", [](const telescope* tel, const arguments_t& args) {
      coord_t coord = coord_t::parse(args);
      return static_cast<int>(tel->get_destinationsideofpier(coord.rightascension, coord.declination));
    });

    // constants
    define_get("alignmentmode", [](const telescope* tel, const arguments_t& args) {
      return static_cast<int>(tel->get_alignmentmode());
    });
    define_get("aperturearea", [](const telescope* tel, const arguments_t& args) {
      return tel->get_aperturearea();
    });
    define_get("aperturediameter", [](const telescope* tel, const arguments_t& args) {
      return tel->get_aperturediameter();
    });
    define_get("focallength", [](const telescope* tel, const arguments_t& args) {
      return tel->get_focallength();
    });
    define_get("equatorialsystem", [](const telescope* tel, const arguments_t& args) {
      return static_cast<int>(tel->get_equatorialsystem());
    });
    define_get("axisrates", [](const telescope* tel, const arguments_t& args) {
      int axis = parser::parser_t::parse<int>(args, fields::axis_f);

      json_array axisrates;
      for (const auto& r : tel->get_axisrates(axis)) {
        json_object rate = {
          {"Minimum", r.minimum},
          {"Maximum", r.maximum}
        };
        axisrates.push_back(rate);
      }
      return axisrates;
    });
    define_get("trackingrates", [](const telescope* tel, const arguments_t& args) {
      auto trackingrates = tel->get_trackingrates();
      json_array out_trackingrates;

      std::transform(
        std::cbegin(trackingrates),
        std::cend(trackingrates),
        std::back_inserter(out_trackingrates),
        [](driver_rate_t rate) {
          return static_cast<int>(rate);
        }
      );

      return out_trackingrates;
    });

    // flags
    define_get("canfindhome", [](const telescope* tel, const arguments_t& args) {
      return tel->get_canfindhome();
    });
    define_get("canmoveaxis", [](const telescope* tel, const arguments_t& args) {
      int axis = parser::parser_t::parse<int>(args, fields::axis_f);
      return tel->get_canmoveaxis(axis);
    });
    define_get("canpark", [](const telescope* tel, const arguments_t& args) {
      return tel->get_canpark();
    });
    define_get("canpulseguide", [](const telescope* tel, const arguments_t& args) {
      return tel->get_canpulseguide();
    });
    define_get("cansetdeclinationrate", [](const telescope* tel, const arguments_t& args) {
      return tel->get_cansetdeclinationrate();
    });
    define_get("cansetguiderates", [](const telescope* tel, const arguments_t& args) {
      return tel->get_cansetguiderates();
    });
    define_get("cansetpark", [](const telescope* tel, const arguments_t& args) {
      return tel->get_cansetpark();
    });
    define_get("cansetpierside", [](const telescope* tel, const arguments_t& args) {
      return tel->get_cansetpierside();
    });
    define_get("cansetrightascensionrate", [](const telescope* tel, const arguments_t& args) {
      return tel->get_cansetrightascensionrate();
    });
    define_get("cansettracking", [](const telescope* tel, const arguments_t& args) {
      return tel->get_cansettracking();
    });
    define_get("canslew", [](const telescope* tel, const arguments_t& args) {
      return tel->get_canslew();
    });
    define_get("canslewaltaz", [](const telescope* tel, const arguments_t& args) {
      return tel->get_canslewaltaz();
    });
    define_get("canslewaltazasync", [](const telescope* tel, const arguments_t& args) {
      return tel->get_canslewaltazasync();
    });
    define_get("canslewasync", [](const telescope* tel, const arguments_t& args) {
      return tel->get_canslewasync();
    });
    define_get("cansync", [](const telescope* tel, const arguments_t& args) {
      return tel->get_cansync();
    });
    define_get("cansyncaltaz", [](const telescope* tel, const arguments_t& args) {
      return tel->get_cansyncaltaz();
    });
    define_get("canunpark", [](const telescope* tel, const arguments_t& args) {
      return tel->get_canunpark();
    });

    // read-wrie properties
    define_ops(
      "declinationrate",
      [](const telescope* tel, const arguments_t& args) {
        return tel->get_declinationrate();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->put_declinationrate(
          parser::parser_t::parse<float>(args, fields::declinationrate_f));
      });
    define_ops(
      "doesrefraction",
      [](const telescope* tel, const arguments_t& args) {
        return tel->get_doesrefraction();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->put_doesrefraction(
          parser::parser_t::parse<bool>(args, fields::doesrefraction_f));
      });
    define_ops(
      "guideratedeclination",
      [](const telescope* tel, const arguments_t& args) {
        return tel->get_guideratedeclination();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->put_guideratedeclination(
          parser::parser_t::parse<float>(args, fields::guideratedeclination_f));
      });
    define_ops(
      "guideraterightascension",
      [](const telescope* tel, const arguments_t& args) {
        return tel->get_guideraterightascension();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->put_guideraterightascension(
          parser::parser_t::parse<float>(args, fields::guideraterightascension_f));
      });
    define_ops(
      "rightascensionrate",
      [](const telescope* tel, const arguments_t& args) {
        return tel->get_rightascensionrate();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->put_rightascensionrate(
          parser::parser_t::parse<float>(args, fields::rightascensionrate_f));
      });
    define_ops(
      "sideofpier",
      [](const telescope* tel, const arguments_t& args) {
        return tel->get_sideofpier();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->put_sideofpier(parser::parser_t::parse<int>(args, fields::sideofpier_f));
      });
    define_ops(
      "siteelevation",
      [](const telescope* tel, const arguments_t& args) {
        return tel->get_siteelevation();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->put_siteelevation(
          parser::parser_t::parse<float>(args, fields::siteelevation_f));
      });
    define_ops(
      "sitelatitude",
      [](const telescope* tel, const arguments_t& args) {
        return tel->get_sitelatitude();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->put_sitelatitude(
          parser::parser_t::parse<float>(args, fields::sitelatitude_f));
      });
    define_ops(
      "sitelongitude",
      [](const telescope* tel, const arguments_t& args) {
        return tel->get_sitelongitude();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->put_sitelongitude(
          parser::parser_t::parse<float>(args, fields::sitelongitude_f));
      });
    define_ops(
      "slewsettletime",
      [](const telescope* tel, const arguments_t& args) {
        return tel->get_slewsettletime();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->put_slewsettletime(
          parser::parser_t::parse<int>(args, fields::slewsettletime_f));
      });
    define_ops(
      "targetdeclination",
      [](const telescope* tel, const arguments_t& args) {
        return tel->get_targetdeclination();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->put_targetdeclination(
          parser::parser_t::parse<float>(args, fields::targetdeclination_f));
      });
    define_ops(
      "targetrightascension",
      [](const telescope* tel, const arguments_t& args) {
        return tel->get_targetrightascension();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->put_targetrightascension(
          parser::parser_t::parse<float>(args, fields::targetrightascension_f));
      });
    define_ops(
      "tracking",
      [](const telescope* tel, const arguments_t& args) {
        return tel->get_tracking();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->put_tracking(parser::parser_t::parse<bool>(args, fields::tracking_f));
      });
    define_ops(
      "trackingrate",
      [](const telescope* tel, const arguments_t& args) {
        return static_cast<int>(tel->get_trackingrate());
      },
      [](telescope* tel, const arguments_t& args) {
        tel->put_trackingrate(
          parser::parser_t::parse<driver_rate_t>(args, fields::trackingrate_f));
      });
    define_ops(
      "utcdate",
      [](const telescope* tel, const arguments_t& args) {
        return tel->get_utcdate();
      },
      [](telescope* tel, const arguments_t& args) {
        auto utc = parser::parser_t::parse<std::string>(args, fields::utcdate_f);
        tel->put_utcdate(utc);
      });

    // operations
    define_put("abortslew", [](telescope* tel, const arguments_t& args) {
      tel->abortslew();
    });
    define_put("findhome", [](telescope* tel, const arguments_t& args) {
      tel->findhome();
    });
    define_put("setpark", [](telescope* tel, const arguments_t& args) {
      tel->setpark();
    });
    define_put("park", [](telescope* tel, const arguments_t& args) {
      tel->park();
    });
    define_put("slewtotarget", [](telescope* tel, const arguments_t& args) {
      tel->slewtotarget();
    });
    define_put("slewtotargetasync", [](telescope* tel, const arguments_t& args) {
      tel->slewtotargetasync();
    });
    define_put("synctotarget", [](telescope* tel, const arguments_t& args) {
      tel->synctotarget();
    });
    define_put("unpark", [](telescope* tel, const arguments_t& args) {
      tel->unpark();
    });
    define_put("moveaxis", [](telescope* tel, const arguments_t& args) {
      move_t move = move_t::parse(args);
      tel->moveaxis(move.axis, move.rate);
    });
    define_put("pulseguide", [](telescope* tel, const arguments_t& args) {
      pulse_t pulse = pulse_t::parse(args);
      tel->pulseguide(pulse.direction, pulse.duration);
    });
    define_put("slewtoaltaz", [](telescope* tel, const arguments_t& args) {
      altazm_t altazm = altazm_t::parse(args);
      tel->slewtoaltaz(altazm.altitude, altazm.azimuth);
    });
    define_put("slewtoaltazasync", [](telescope* tel, const arguments_t& args) {
      altazm_t altazm = altazm_t::parse(args);
      tel->slewtoaltazasync(altazm.altitude, altazm.azimuth);
    });
    define_put("slewtocoordinates", [](telescope* tel, const arguments_t& args) {
      coord_t coord = coord_t::parse(args);
      tel->slewtocoordinates(coord.rightascension, coord.declination);
    });
    define_put("slewtocoordinatesasync", [](telescope* tel, const arguments_t& args) {
      coord_t coord = coord_t::parse(args);
      tel->slewtocoordinatesasync(coord.rightascension, coord.declination);
    });
    define_put("synctoaltaz", [](telescope* tel, const arguments_t& args) {
      altazm_t altazm = altazm_t::parse(args);
      tel->synctoaltaz(altazm.altitude, altazm.azimuth);
    });
    define_put("synctocoordinates", [](telescope* tel, const arguments_t& args) {
      coord_t coord = coord_t::parse(args);
      tel->synctocoordinates(coord.rightascension, coord.declination);
    });
  }
};

class telescope_setup_resource : public httpserver::http_resource {
  telescope_resource* resource;
 public:
  telescope_setup_resource(telescope_resource* resource)
  : resource(resource) { }

  virtual std::shared_ptr<httpserver::http_response> render(const httpserver::http_request& req) override {
    (void)resource;

    json_object obj = {
      {"device_type", req.get_path_piece(2)},
      {"device_number", req.get_path_piece(3)},
      {"operation", req.get_path_piece(4)},
    };

    std::ostringstream os;
    os << obj;

    return std::make_shared<httpserver::string_response>(os.str(), 200, "application/json");
  }
};

}  // namespace alpaca

#endif  // INCLUDE_TELESCOPE_HPP_
