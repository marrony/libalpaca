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
#include <time.hpp>
#include <json.hpp>

namespace alpaca {

enum class driver_rate_t : int {
  sidereal = 0,
  lunar,
  solar,
  king
};

struct driver_rate_arcs_t {
  const float sidereal = 15.0410;
  const float lunar    = 14.6850;
  const float solar    = 15.0000;
  const float king     = 15.0369;
};

struct axis_rate_t {
  float minimum;
  float maximum;
};

enum class equatorial_system_t : std::int32_t {
  other = 0,
  jnow  = 1,
  j2000 = 2,
  j2050 = 3,
  b1950 = 4
};

enum class alignment_mode_t : std::int32_t {
  alt_azm = 0,
  polar   = 1,
  german  = 2
};

enum class destination_side_of_pier_t : std::int32_t {
  pier_unknown = -1,
  pier_east    = 0,
  pier_west    = 1,
};

struct telescope_flags_t {
  enum {
    none                         = 0x00000,
    can_find_home                = 0x00001,
    can_park                     = 0x00002,
    can_pulse_guide              = 0x00004,
    can_set_declination_rate     = 0x00008,
    can_set_guide_rates          = 0x00010,
    can_set_park                 = 0x00020,
    can_set_pier_side            = 0x00040,
    can_set_right_ascension_rate = 0x00080,
    can_set_tracking             = 0x00100,
    can_slew                     = 0x00200,
    can_slew_altaz               = 0x00400,
    can_slew_altaz_async         = 0x00800,
    can_slew_async               = 0x01000,
    can_sync                     = 0x02000,
    can_sync_altaz               = 0x04000,
    can_unpark                   = 0x08000,
    can_move_axis_0              = 0x10000,
    can_move_axis_1              = 0x20000,
    can_move_axis_2              = 0x40000,
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

  static result<altazm_t, alpaca_error> parse(const arguments_t& args) {
    return parser::parser_t::parse<altazm_t>(args, fields::altitude_f, fields::azimuth_f);
  }
};

struct coord_t {
  const float rightascension;
  const float declination;

  coord_t(float rightascension, float declination)
  : rightascension(rightascension), declination(declination) { }

  static result<coord_t, alpaca_error> parse(const arguments_t& args) {
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

  static result<pulse_t, alpaca_error> parse(const arguments_t& args) {
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

  static result<move_t, alpaca_error> parse(const arguments_t& args) {
    return parser::parser_t::parse<move_t>(args, fields::axis_f, fields::rate_f);
  }
};

class telescope : public device {
  telescopeinfo_t telescopeinfo;

  inline void check_parked() const {
    if (get_atpark()) throw parked();
  }

 public:
  // resource needs to be able to call telescope private methods
  friend class telescope_resource;

  // read-only properties
  auto priv_get_altitude() const {
    return check_connected()
      .map([=]() {
        return get_altitude();
      });
  }

  auto priv_get_azimuth() const {
    return check_connected()
      .map([=]() {
        return get_azimuth();
      });
  }

  auto priv_get_declination() const {
    return check_connected()
      .map([=]() {
        return get_declination();
      });
  }

  auto priv_get_rightascension() const {
    return check_connected()
      .map([=]() {
        return get_rightascension();
      });
  }

  auto priv_get_athome() const {
    return check_connected()
      .map([=]() {
        return get_athome();
      });
  }

  auto priv_get_atpark() const {
    return check_connected()
      .map([=]() {
        return get_atpark();
      });
  }

  auto priv_get_ispulseguiding() const {
    return check_connected()
      .map([=]() {
        check_flag(get_canpulseguide());
        return get_ispulseguiding();
      });
  }

  auto priv_get_slewing() const {
    return check_connected()
      .map([=]() {
        return get_slewing();
      });
  }

  auto priv_get_siderealtime() const {
    return check_connected()
      .map([=]() {
        return get_siderealtime();
      });
  }

  auto priv_get_destinationsideofpier(
    float rightascension, float declination) const {
    return check_connected()
      .map([=]() {
        return get_destinationsideofpier(rightascension, declination);
      });
  }

  // read-wrie properties
  auto priv_get_declinationrate() const {
    return check_connected()
      .map([=]() {
        return get_declinationrate();
      });
  }

  auto priv_put_declinationrate(float declinationrate) {
    return check_connected()
      .map([=]() {
        check_flag(get_cansetdeclinationrate());
        put_declinationrate(declinationrate);
      });
  }

  auto priv_get_rightascensionrate() const {
    return check_connected()
      .map([=]() {
        return get_rightascensionrate();
      });
  }

  auto priv_put_rightascensionrate(float rightascensionrate) {
    return check_connected()
      .map([=]() {
        check_flag(get_cansetrightascensionrate());
        put_rightascensionrate(rightascensionrate);
      });
  }

  auto priv_get_doesrefraction() const {
    return check_connected()
      .map([=]() {
        return get_doesrefraction();
      });
  }

  auto priv_put_doesrefraction(bool doesrefraction) {
    return check_connected()
      .map([=]() {
        put_doesrefraction(doesrefraction);
      });
  }

  auto priv_get_guideratedeclination() const {
    return check_connected()
      .map([=]() {
        return get_guideratedeclination();
      });
  }

  auto priv_put_guideratedeclination(float guideratedeclination) {
    return check_connected()
      .map([=]() {
        check_flag(get_cansetguiderates());
        put_guideratedeclination(guideratedeclination);
      });
  }

  auto priv_get_guideraterightascension() const {
    return check_connected()
      .map([=]() {
        return get_guideraterightascension();
      });
  }

  auto priv_put_guideraterightascension(float guideraterightascension) {
    return check_connected()
      .map([=]() {
        check_flag(get_cansetguiderates());
        put_guideraterightascension(guideraterightascension);
      });
  }

  auto priv_get_sideofpier() const {
    return check_connected()
      .map([=]() {
        return get_sideofpier();
      });
  }

  auto priv_put_sideofpier(int sideofpier) {
    return check_connected()
      .map([=]() {
        put_sideofpier(sideofpier);
      });
  }

  auto priv_get_siteelevation() const {
    return check_connected()
      .map([=]() {
        return get_siteelevation();
      });
  }

  auto priv_put_siteelevation(float elevation) {
    return check_connected()
      .map([=]() {
        check_value(elevation >= -300.0f && elevation <= 10000.0f);
        put_siteelevation(elevation);
      });
  }

  auto priv_get_sitelatitude() const {
    return check_connected()
      .map([=]() {
        return get_sitelatitude();
      });
  }

  auto priv_put_sitelatitude(float latitude) {
    return check_connected()
      .map([=]() {
        check_value(latitude >= -90.0f && latitude <= +90.0f);
        put_sitelatitude(latitude);
      });
  }

  auto priv_get_sitelongitude() const {
    return check_connected()
      .map([=]() {
        return get_sitelongitude();
      });
  }

  auto priv_put_sitelongitude(float angle) {
    return check_connected()
      .map([=]() {
        check_value(angle >= -180.0f && angle <= +180.0f);
        put_sitelongitude(angle);
      });
  }


  auto priv_get_slewsettletime() const {
    return check_connected()
      .map([=]() {
        return get_slewsettletime();
      });
  }

  auto priv_put_slewsettletime(int slewsettletime) {
    return check_connected()
      .map([=]() {
        check_value(slewsettletime >= 0);
        put_slewsettletime(slewsettletime);
      });
  }

  auto priv_get_targetdeclination() const {
    return check_connected()
      .map([=]() {
        return get_targetdeclination();
      });
  }

  auto priv_put_targetdeclination(float targetdeclination) {
    return check_connected()
      .map([=]() {
        check_value(targetdeclination >= -90.0f && targetdeclination <= +90.0f);
        put_targetdeclination(targetdeclination);
      });
  }

  auto priv_get_targetrightascension() const {
    return check_connected()
      .map([=]() {
        return get_targetrightascension();
      });
  }

  auto priv_put_targetrightascension(float targetrightascension) {
    return check_connected()
      .map([=]() {
        check_value(targetrightascension >= 0.0f && targetrightascension <= +24.0f);
        put_targetrightascension(targetrightascension);
      });
  }

  auto priv_get_tracking() const {
    return check_connected()
      .map([=]() {
        return get_tracking();
      });
  }

  auto priv_put_tracking(bool tracking) {
    return check_connected()
      .map([=]() {
        put_tracking(tracking);
      });
  }

  auto priv_get_trackingrate() const {
    return check_connected()
      .map([=]() {
        return get_trackingrate();
      });
  }

  auto priv_put_trackingrate(int rate) {
    return check_connected()
      .map([=]() {
        check_value(rate >= 0 && rate <= 3);
        put_trackingrate(static_cast<driver_rate_t>(rate));
      });
  }

  auto priv_get_utcdate() const {
    return check_connected()
      .map([=]() {
        return get_utcdate();
      });
  }

  auto priv_put_utcdate(const std::string& utc) {
    return check_connected()
      .map([=]() {
        put_utcdate(utc);
      });
  }

  // operations
  auto priv_abortslew() {
    return check_connected()
      .map([=]() {
        abortslew();
      });
  }

  auto priv_findhome() {
    return check_connected()
      .map([=]() {
        check_flag(get_canfindhome());

        findhome();
      });
  }

  auto priv_moveaxis(int axis, float rate) {
    return check_connected()
      .map([=]() {
        check_flag(get_canmoveaxis(axis));
        check_value(rate > -9.0f && rate < +9.0f);

        moveaxis(axis, rate);
      });
  }

  auto priv_park() {
    return check_connected()
      .map([=]() {
        check_flag(get_canpark());

        park();
      });
  }

  auto priv_pulseguide(int direction, int duration) {
    return check_connected()
      .map([=]() {
        check_flag(get_canpulseguide());

        pulseguide(direction, duration);
      });
  }

  auto priv_setpark() {
    return check_connected()
      .map([=]() {
        check_flag(get_cansetpark());

        setpark();
      });
  }

  auto priv_slewtoaltaz(float altitude, float azimuth) {
    return check_connected()
      .map([=]() {
        check_flag(get_canslewaltaz());

        slewtoaltaz(altitude, azimuth);
      });
  }

  auto priv_slewtoaltazasync(float altitude, float azimuth) {
    return check_connected()
      .map([=]() {
        check_flag(get_canslewaltazasync());
        check_value(azimuth >= 0.0f && azimuth <= 360.f);
        check_value(altitude >= -90.0f && altitude <= +90.f);

        slewtoaltazasync(altitude, azimuth);
      });
  }

  auto priv_slewtocoordinates(
    float rightascension, float declination) {
    return check_connected()
      .map([=]() {
        check_flag(get_canslew());

        slewtocoordinates(rightascension, declination);
      });
  }

  auto priv_slewtocoordinatesasync(
    float rightascension, float declination) {
    return check_connected()
      .map([=]() {
        check_flag(get_canslewasync());
        check_value(declination >= -90.0f && declination <= +90.0f);
        check_value(rightascension >= 0.0f && rightascension <= +24.0f);

        slewtocoordinatesasync(rightascension, declination);
      });
  }

  auto priv_slewtotarget() {
    return check_connected()
      .map([=]() {
        check_flag(get_canslew());

        slewtotarget();
      });
  }

  auto priv_slewtotargetasync() {
    return check_connected()
      .map([=]() {
        check_flag(get_canslewasync());

        slewtotargetasync();
      });
  }

  auto priv_synctoaltaz(float altitude, float azimuth) {
    return check_connected()
      .map([=]() {
        check_flag(get_cansyncaltaz());
        check_value(azimuth >= 0.0f && azimuth <= 360.f);
        check_value(altitude >= -90.0f && altitude <= +90.f);

        synctoaltaz(altitude, azimuth);
      });
  }

  auto priv_synctocoordinates(
    float rightascension, float declination) {
    return check_connected()
      .map([=]() {
        check_flag(get_cansync());
        check_value(declination >= -90.0f && declination <= +90.0f);
        check_value(rightascension >= 0.0f && rightascension <= +24.0f);

        synctocoordinates(rightascension, declination);
      });
  }

  auto priv_synctotarget() {
    return check_connected()
      .map([=]() {
        check_parked();
        check_flag(get_cansync());

        synctotarget();
      });
  }

  auto priv_unpark() {
    return check_connected()
      .map([=]() {
        check_flag(get_canunpark());

        unpark();
      });
  }

 public:
  telescope(const telescopeinfo_t& telescopeinfo)
  : device()
  , telescopeinfo(telescopeinfo)
  { }

  virtual ~telescope()
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
  virtual destination_side_of_pier_t get_destinationsideofpier(float rightascension, float declination) const = 0;

  // constants
  virtual std::string get_description() const { return telescopeinfo.description; }
  virtual std::string get_driverinfo() const { return telescopeinfo.driverinfo; }
  virtual std::string get_driverversion() const { return telescopeinfo.driverversion; }
  virtual int get_interfaceversion() const { return telescopeinfo.interfaceversion; }
  virtual std::string get_name() const { return telescopeinfo.name; }
  virtual const std::vector<std::string> get_supportedactions() const { return { }; }
  virtual alignment_mode_t get_alignmentmode() const { return telescopeinfo.alignmentmode; }
  virtual float get_aperturearea() const { return telescopeinfo.aperturearea; }
  virtual float get_aperturediameter() const { return telescopeinfo.aperturediameter; }
  virtual float get_focallength() const { return telescopeinfo.focallength; }
  virtual equatorial_system_t get_equatorialsystem() const {
    return telescopeinfo.equatorialsystem;
  }
  virtual const std::vector<axis_rate_t> get_axisrates(int axis) const {
    check_value(axis >= 0 && axis <= 2);
    return telescopeinfo.axisrates;
  }
  virtual const std::vector<driver_rate_t> get_trackingrates() const {
    return telescopeinfo.trackingrates;
  }

  // read-wrie properties
  virtual float get_declinationrate() const { return 0; }
  virtual void put_declinationrate(float) { }
  virtual float get_rightascensionrate() const { return 0; }
  virtual void put_rightascensionrate(float) { }
  virtual bool get_doesrefraction() const { return false; }
  virtual void put_doesrefraction(bool) { }
  virtual float get_guideratedeclination() const { return 0; }
  virtual void put_guideratedeclination(float) { }
  virtual float get_guideraterightascension() const { return 0; }
  virtual void put_guideraterightascension(float) { }
  virtual int get_sideofpier() const { return 0; }
  virtual void put_sideofpier(int) { }
  virtual float get_siteelevation() const { return 0; }
  virtual void put_siteelevation(float) { }
  virtual float get_sitelatitude() const { return 0; }
  virtual void put_sitelatitude(float) { }
  virtual float get_sitelongitude() const { return 0; }
  virtual void put_sitelongitude(float) { }
  virtual int get_slewsettletime() const { return 0; }
  virtual void put_slewsettletime(int) { }
  virtual float get_targetdeclination() const { return 0; }
  virtual void put_targetdeclination(float) { }
  virtual float get_targetrightascension() const { return 0; }
  virtual void put_targetrightascension(float) { }
  virtual bool get_tracking() const { return false; }
  virtual void put_tracking(bool) { }
  virtual driver_rate_t get_trackingrate() const { return driver_rate_t::sidereal; }
  virtual void put_trackingrate(driver_rate_t rate) { }

  virtual std::string get_utcdate() const {
    utcdate_t utcdate;

    get_utctm(&utcdate);
    return utcdate.format_utc();
  }

  virtual void put_utcdate(const std::string& utc) {
    put_utctm(utcdate_t::parse_utc(utc).get());
  }

  virtual void get_utctm(utcdate_t*) const = 0;
  virtual void put_utctm(utcdate_t) = 0;

  // flags
  bool get_canfindhome() const {
    return (telescopeinfo.flags & telescope_flags_t::can_find_home);
  }
  bool get_canmoveaxis(int axis) const {
    check_value(axis >= 0 && axis <= 2);
    return telescopeinfo.flags & (telescope_flags_t::can_move_axis_0 << axis);
  }
  bool get_canpark() const {
    return telescopeinfo.flags & telescope_flags_t::can_park;
  }
  bool get_canpulseguide() const {
    return telescopeinfo.flags & telescope_flags_t::can_pulse_guide;
  }
  bool get_cansetdeclinationrate() const {
    return telescopeinfo.flags & telescope_flags_t::can_set_declination_rate;
  }
  bool get_cansetguiderates() const {
    return telescopeinfo.flags & telescope_flags_t::can_set_guide_rates;
  }
  bool get_cansetpark() const {
    return telescopeinfo.flags & telescope_flags_t::can_set_park;
  }
  bool get_cansetpierside() const {
    return telescopeinfo.flags & telescope_flags_t::can_set_pier_side;
  }
  bool get_cansetrightascensionrate() const {
    return telescopeinfo.flags & telescope_flags_t::can_set_right_ascension_rate;
  }
  bool get_cansettracking() const {
    return telescopeinfo.flags & telescope_flags_t::can_set_tracking;
  }
  bool get_canslew() const {
    return telescopeinfo.flags & telescope_flags_t::can_slew;
  }
  bool get_canslewaltaz() const {
    return telescopeinfo.flags & telescope_flags_t::can_slew_altaz;
  }
  bool get_canslewaltazasync() const {
    return telescopeinfo.flags & telescope_flags_t::can_slew_altaz_async;
  }
  bool get_canslewasync() const {
    return telescopeinfo.flags & telescope_flags_t::can_slew_async;
  }
  bool get_cansync() const {
    return telescopeinfo.flags & telescope_flags_t::can_sync;
  }
  bool get_cansyncaltaz() const {
    return telescopeinfo.flags & telescope_flags_t::can_sync_altaz;
  }
  bool get_canunpark() const {
    return telescopeinfo.flags & telescope_flags_t::can_unpark;
  }

  // operations
  virtual void abortslew() = 0;
  virtual void findhome() = 0;
  virtual void moveaxis(int axis, float rate) = 0;
  virtual void park() = 0;
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
    define_get("altitude", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->priv_get_altitude();
    });
    define_get("azimuth", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->priv_get_azimuth();
    });
    define_get("declination", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->priv_get_declination();
    });
    define_get("rightascension", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->priv_get_rightascension();
    });
    define_get("athome", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->priv_get_athome();
    });
    define_get("atpark", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->priv_get_atpark();
    });
    define_get("ispulseguiding", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->priv_get_ispulseguiding();
    });
    define_get("slewing", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->priv_get_slewing();
    });
    define_get("siderealtime", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->priv_get_siderealtime();
    });
    define_get("destinationsideofpier", [](const telescope* tel, const arguments_t& args) -> return_t {
      return coord_t::parse(args).flat_map([tel](const coord_t& coord) {
        return tel->priv_get_destinationsideofpier(coord.rightascension, coord.declination)
          .map([](destination_side_of_pier_t d) {
            return static_cast<int>(d);
          });
      });
    });

    // constants
    define_get("alignmentmode", [](const telescope* tel, const arguments_t& args) -> return_t {
      return static_cast<int>(tel->get_alignmentmode());
    });
    define_get("aperturearea", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->get_aperturearea();
    });
    define_get("aperturediameter", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->get_aperturediameter();
    });
    define_get("focallength", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->get_focallength();
    });
    define_get("equatorialsystem", [](const telescope* tel, const arguments_t& args) -> return_t {
      return static_cast<int>(tel->get_equatorialsystem());
    });
    define_get("axisrates", [](const telescope* tel, const arguments_t& args) -> return_t {
      return parser::parser_t::parse<int>(args, fields::axis_f).map([tel](int axis) {
        json_array axisrates;
        for (const auto& r : tel->get_axisrates(axis)) {
          json_object rate = {
            {"Minimum", r.minimum},
            {"Maximum", r.maximum}
          };
          axisrates.push_back(rate);
        }
        return static_cast<json_value>(axisrates);
      });
    });
    define_get("trackingrates", [](const telescope* tel, const arguments_t& args) -> return_t {
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

      return static_cast<json_value>(out_trackingrates);
    });

    // flags
    define_get("canfindhome", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->get_canfindhome();
    });
    define_get("canmoveaxis", [](const telescope* tel, const arguments_t& args) -> return_t {
      return parser::parser_t::parse<int>(args, fields::axis_f).map([tel](int axis) {
        return tel->get_canmoveaxis(axis);
      });
    });
    define_get("canpark", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->get_canpark();
    });
    define_get("canpulseguide", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->get_canpulseguide();
    });
    define_get("cansetdeclinationrate", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->get_cansetdeclinationrate();
    });
    define_get("cansetguiderates", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->get_cansetguiderates();
    });
    define_get("cansetpark", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->get_cansetpark();
    });
    define_get("cansetpierside", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->get_cansetpierside();
    });
    define_get("cansetrightascensionrate", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->get_cansetrightascensionrate();
    });
    define_get("cansettracking", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->get_cansettracking();
    });
    define_get("canslew", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->get_canslew();
    });
    define_get("canslewaltaz", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->get_canslewaltaz();
    });
    define_get("canslewaltazasync", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->get_canslewaltazasync();
    });
    define_get("canslewasync", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->get_canslewasync();
    });
    define_get("cansync", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->get_cansync();
    });
    define_get("cansyncaltaz", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->get_cansyncaltaz();
    });
    define_get("canunpark", [](const telescope* tel, const arguments_t& args) -> return_t {
      return tel->get_canunpark();
    });

    // read-wrie properties
    define_ops(
      "declinationrate",
      [](const telescope* tel, const arguments_t& args) -> return_t {
        return tel->priv_get_declinationrate();
      },
      [](telescope* tel, const arguments_t& args) -> return_void_t {
        return parser::parser_t::parse<float>(args, fields::declinationrate_f)
          .map([tel](float declinationrate) {
            tel->priv_put_declinationrate(declinationrate);
          });
      });
    define_ops(
      "doesrefraction",
      [](const telescope* tel, const arguments_t& args) -> return_t {
        return tel->priv_get_doesrefraction();
      },
      [](telescope* tel, const arguments_t& args) -> return_void_t {
        return tel->priv_put_doesrefraction(
          parser::parser_t::parse<bool>(args, fields::doesrefraction_f).get());
      });
    define_ops(
      "guideratedeclination",
      [](const telescope* tel, const arguments_t& args) -> return_t {
        return tel->priv_get_guideratedeclination();
      },
      [](telescope* tel, const arguments_t& args) -> return_void_t {
        return tel->priv_put_guideratedeclination(
          parser::parser_t::parse<float>(args, fields::guideratedeclination_f).get());
      });
    define_ops(
      "guideraterightascension",
      [](const telescope* tel, const arguments_t& args) -> return_t {
        return tel->priv_get_guideraterightascension();
      },
      [](telescope* tel, const arguments_t& args) -> return_void_t {
        return tel->priv_put_guideraterightascension(
          parser::parser_t::parse<float>(args, fields::guideraterightascension_f).get());
      });
    define_ops(
      "rightascensionrate",
      [](const telescope* tel, const arguments_t& args) -> return_t {
        return tel->priv_get_rightascensionrate();
      },
      [](telescope* tel, const arguments_t& args) -> return_void_t {
        return tel->priv_put_rightascensionrate(
          parser::parser_t::parse<float>(args, fields::rightascensionrate_f).get());
      });
    define_ops(
      "sideofpier",
      [](const telescope* tel, const arguments_t& args) -> return_t {
        return tel->priv_get_sideofpier();
      },
      [](telescope* tel, const arguments_t& args) -> return_void_t {
        return tel->priv_put_sideofpier(
          parser::parser_t::parse<int>(args, fields::sideofpier_f).get());
      });
    define_ops(
      "siteelevation",
      [](const telescope* tel, const arguments_t& args) -> return_t {
        return tel->priv_get_siteelevation();
      },
      [](telescope* tel, const arguments_t& args) -> return_void_t {
        return tel->priv_put_siteelevation(
          parser::parser_t::parse<float>(args, fields::siteelevation_f).get());
      });
    define_ops(
      "sitelatitude",
      [](const telescope* tel, const arguments_t& args) -> return_t {
        return tel->priv_get_sitelatitude();
      },
      [](telescope* tel, const arguments_t& args) -> return_void_t {
        return tel->priv_put_sitelatitude(
          parser::parser_t::parse<float>(args, fields::sitelatitude_f).get());
      });
    define_ops(
      "sitelongitude",
      [](const telescope* tel, const arguments_t& args) -> return_t {
        return tel->priv_get_sitelongitude();
      },
      [](telescope* tel, const arguments_t& args) -> return_void_t {
        return tel->priv_put_sitelongitude(
          parser::parser_t::parse<float>(args, fields::sitelongitude_f).get());
      });
    define_ops(
      "slewsettletime",
      [](const telescope* tel, const arguments_t& args) -> return_t {
        return tel->priv_get_slewsettletime();
      },
      [](telescope* tel, const arguments_t& args) -> return_void_t {
        return tel->priv_put_slewsettletime(
          parser::parser_t::parse<int>(args, fields::slewsettletime_f).get());
      });
    define_ops(
      "targetdeclination",
      [](const telescope* tel, const arguments_t& args) -> return_t {
        return tel->priv_get_targetdeclination();
      },
      [](telescope* tel, const arguments_t& args) -> return_void_t {
        return tel->priv_put_targetdeclination(
          parser::parser_t::parse<float>(args, fields::targetdeclination_f).get());
      });
    define_ops(
      "targetrightascension",
      [](const telescope* tel, const arguments_t& args) -> return_t {
        return tel->priv_get_targetrightascension();
      },
      [](telescope* tel, const arguments_t& args) -> return_void_t {
        return tel->priv_put_targetrightascension(
          parser::parser_t::parse<float>(args, fields::targetrightascension_f).get());
      });
    define_ops(
      "tracking",
      [](const telescope* tel, const arguments_t& args) -> return_t {
        return tel->priv_get_tracking();
      },
      [](telescope* tel, const arguments_t& args) -> return_void_t {
        return tel->priv_put_tracking(
          parser::parser_t::parse<bool>(args, fields::tracking_f).get());
      });
    define_ops(
      "trackingrate",
      [](const telescope* tel, const arguments_t& args) -> return_t {
        return tel->priv_get_trackingrate().map([](driver_rate_t t) {
          return static_cast<int>(t);
        });
      },
      [](telescope* tel, const arguments_t& args) -> return_void_t {
        return tel->priv_put_trackingrate(
          parser::parser_t::parse<int>(args, fields::trackingrate_f).get());
      });
    define_ops(
      "utcdate",
      [](const telescope* tel, const arguments_t& args) -> return_t {
        return tel->priv_get_utcdate();
      },
      [](telescope* tel, const arguments_t& args) -> return_void_t {
        auto utc = parser::parser_t::parse<std::string_view>(args, fields::utcdate_f).get();
        return tel->priv_put_utcdate(std::string{utc});
      });

    // operations
    define_put("abortslew", [](telescope* tel, const arguments_t& args) -> return_void_t {
      tel->priv_abortslew();
      return return_void_t{};
    });
    define_put("findhome", [](telescope* tel, const arguments_t& args) -> return_void_t {
      tel->priv_findhome();
      return return_void_t{};
    });
    define_put("setpark", [](telescope* tel, const arguments_t& args) -> return_void_t {
      tel->priv_setpark();
      return return_void_t{};
    });
    define_put("park", [](telescope* tel, const arguments_t& args) -> return_void_t {
      tel->priv_park();
      return return_void_t{};
    });
    define_put("slewtotarget", [](telescope* tel, const arguments_t& args) -> return_void_t {
      tel->priv_slewtotarget();
      return return_void_t{};
    });
    define_put("slewtotargetasync", [](telescope* tel, const arguments_t& args) -> return_void_t {
      tel->priv_slewtotargetasync();
      return return_void_t{};
    });
    define_put("synctotarget", [](telescope* tel, const arguments_t& args) -> return_void_t {
      tel->priv_synctotarget();
      return return_void_t{};
    });
    define_put("unpark", [](telescope* tel, const arguments_t& args) -> return_void_t {
      tel->priv_unpark();
      return return_void_t{};
    });
    define_put("moveaxis", [](telescope* tel, const arguments_t& args) -> return_void_t {
      move_t move = move_t::parse(args).get();
      tel->priv_moveaxis(move.axis, move.rate);
      return return_void_t{};
    });
    define_put("pulseguide", [](telescope* tel, const arguments_t& args) -> return_void_t {
      pulse_t pulse = pulse_t::parse(args).get();
      tel->priv_pulseguide(pulse.direction, pulse.duration);
      return return_void_t{};
    });
    define_put("slewtoaltaz", [](telescope* tel, const arguments_t& args) -> return_void_t {
      altazm_t altazm = altazm_t::parse(args).get();
      tel->priv_slewtoaltaz(altazm.altitude, altazm.azimuth);
      return return_void_t{};
    });
    define_put("slewtoaltazasync", [](telescope* tel, const arguments_t& args) -> return_void_t {
      altazm_t altazm = altazm_t::parse(args).get();
      tel->priv_slewtoaltazasync(altazm.altitude, altazm.azimuth);
      return return_void_t{};
    });
    define_put("slewtocoordinates", [](telescope* tel, const arguments_t& args) -> return_void_t {
      coord_t coord = coord_t::parse(args).get();
      tel->priv_slewtocoordinates(coord.rightascension, coord.declination);
      return return_void_t{};
    });
    define_put("slewtocoordinatesasync", [](telescope* tel, const arguments_t& args) -> return_void_t {
      coord_t coord = coord_t::parse(args).get();
      tel->priv_slewtocoordinatesasync(coord.rightascension, coord.declination);
      return return_void_t{};
    });
    define_put("synctoaltaz", [](telescope* tel, const arguments_t& args) -> return_void_t {
      altazm_t altazm = altazm_t::parse(args).get();
      tel->priv_synctoaltaz(altazm.altitude, altazm.azimuth);
      return return_void_t{};
    });
    define_put("synctocoordinates", [](telescope* tel, const arguments_t& args) -> return_void_t {
      coord_t coord = coord_t::parse(args).get();
      tel->priv_synctocoordinates(coord.rightascension, coord.declination);
      return return_void_t{};
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
