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

  static return_t<pulse_t> parse(const arguments_t& args) {
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

  static return_t<move_t> parse(const arguments_t& args) {
    return parser::parser_t::parse<move_t>(args, fields::axis_f, fields::rate_f);
  }
};

class telescope : public device {
  telescopeinfo_t telescopeinfo;

  [[nodiscard]]
  inline auto check_parked() const -> check_t {
    return get_atpark()
      .flat_map([](bool atpark) -> check_t {
        if (atpark) return parked();

        return {};
      });
  }

  inline auto check_axis(int axis) const -> check_t {
    return check_value(axis >= 0 && axis <= 2);
  }

 public:
  // resource needs to be able to call telescope private methods
  friend class telescope_resource;

  // read-only properties
  return_t<float> priv_get_altitude() const {
    return visit(
      [this]() {
        return get_altitude();
      },
      check_connected()
    );
  }

  return_t<float> priv_get_azimuth() const {
    return visit(
      [this]() {
        return get_azimuth();
      },
      check_connected()
    );
  }

  return_t<float> priv_get_declination() const {
    return visit(
      [this]() {
        return get_declination();
      },
      check_connected()
    );
  }

  return_t<float> priv_get_rightascension() const {
    return visit(
      [this]() {
        return get_rightascension();
      },
      check_connected()
    );
  }

  return_t<bool> priv_get_athome() const {
    return visit(
      [this]() {
        return get_athome();
      },
      check_connected()
    );
  }

  return_t<bool> priv_get_atpark() const {
    return visit(
      [this]() {
        return get_atpark();
      },
      check_connected()
    );
  }

  return_t<bool> priv_get_ispulseguiding() const {
    return visit(
      [this]() {
        return get_ispulseguiding();
      },
      check_connected(),
      check_flag(get_canpulseguide())
    );
  }

  return_t<bool> priv_get_slewing() const {
    return visit(
      [this]() {
        return get_slewing();
      },
      check_connected()
    );
  }

  return_t<float> priv_get_siderealtime() const {
    return visit(
      [this]() {
        return get_siderealtime();
      },
      check_connected()
    );
  }

  return_t<destination_side_of_pier_t> priv_get_destinationsideofpier(
    float rightascension, float declination) const {
    return visit(
      [this, rightascension, declination]() {
        return get_destinationsideofpier(rightascension, declination);
      },
      check_connected()
    );
  }

  // read-wrie properties
  return_t<float> priv_get_declinationrate() const {
    return visit(
      [this]() {
        return get_declinationrate();
      },
      check_connected()
    );
  }

  return_t<void> priv_put_declinationrate(float declinationrate) {
    return visit(
      [this, declinationrate]() {
        return put_declinationrate(declinationrate);
      },
      check_connected(),
      check_flag(get_cansetdeclinationrate())
    );
  }

  return_t<float> priv_get_rightascensionrate() const {
    return visit(
      [this]() {
        return get_rightascensionrate();
      },
      check_connected()
    );
  }

  return_t<void> priv_put_rightascensionrate(float rightascensionrate) {
    return visit(
      [this, rightascensionrate]() {
        return put_rightascensionrate(rightascensionrate);
      },
      check_connected(),
      check_flag(get_cansetrightascensionrate())
    );
  }

  auto priv_get_doesrefraction() const {
    return visit(
      [this]() {
        return get_doesrefraction();
      },
      check_connected()
    );
  }

  return_t<void> priv_put_doesrefraction(bool doesrefraction) {
    return visit(
      [this, doesrefraction]() {
        return put_doesrefraction(doesrefraction);
      },
      check_connected()
    );
  }

  return_t<float> priv_get_guideratedeclination() const {
    return visit(
      [this]() {
        return get_guideratedeclination();
      },
      check_connected()
    );
  }

  return_t<void> priv_put_guideratedeclination(float guideratedeclination) {
    return visit(
      [this, guideratedeclination]() {
        return put_guideratedeclination(guideratedeclination);
      },
      check_connected(),
      check_flag(get_cansetguiderates())
    );
  }

  return_t<float> priv_get_guideraterightascension() const {
    return visit(
      [this]() {
        return get_guideraterightascension();
      },
      check_connected()
    );
  }

  return_t<void> priv_put_guideraterightascension(float guideraterightascension) {
    return visit(
      [this, guideraterightascension]() {
        return put_guideraterightascension(guideraterightascension);
      },
      check_connected(),
      check_flag(get_cansetguiderates())
    );
  }

  return_t<int> priv_get_sideofpier() const {
    return visit(
      [this]() {
        return get_sideofpier();
      },
      check_connected()
    );
  }

  return_t<void> priv_put_sideofpier(int sideofpier) {
    return visit(
      [this, sideofpier]() {
        return put_sideofpier(sideofpier);
      },
      check_connected()
    );
  }

  return_t<float> priv_get_siteelevation() const {
    return visit(
      [this]() {
        return get_siteelevation();
      },
      check_connected()
    );
  }

  return_t<void> priv_put_siteelevation(float elevation) {
    return visit(
      [this, elevation]() {
        return put_siteelevation(elevation);
      },
      check_connected(),
      check_value(elevation >= -300.0f && elevation <= 10000.0f)
    );
  }

  return_t<float> priv_get_sitelatitude() const {
    return visit(
      [this]() {
        return get_sitelatitude();
      },
      check_connected()
    );
  }

  return_t<void> priv_put_sitelatitude(float latitude) {
    return visit(
      [this, latitude]() {
        return put_sitelatitude(latitude);
      },
      check_connected(),
      check_value(latitude >= -90.0f && latitude <= +90.0f)
    );
  }

  return_t<float> priv_get_sitelongitude() const {
    return visit(
      [this]() {
        return get_sitelongitude();
      },
      check_connected()
    );
  }

  return_t<void> priv_put_sitelongitude(float longitude) {
    return visit(
      [this, longitude]() {
        return put_sitelongitude(longitude);
      },
      check_connected(),
      check_value(longitude >= -180.0f && longitude <= +180.0f)
    );
  }

  return_t<int> priv_get_slewsettletime() const {
    return visit(
      [this]() {
        return get_slewsettletime();
      },
      check_connected()
    );
  }

  return_t<void> priv_put_slewsettletime(int slewsettletime) {
    return visit(
      [this, slewsettletime]() {
        return put_slewsettletime(slewsettletime);
      },
      check_connected(),
      check_value(slewsettletime >= 0)
    );
  }

  return_t<float> priv_get_targetdeclination() const {
    return visit(
      [this]() {
        return get_targetdeclination();
      },
      check_connected()
    );
  }

  return_t<void> priv_put_targetdeclination(float targetdeclination) {
    return visit(
      [this, targetdeclination]() {
        return put_targetdeclination(targetdeclination);
      },
      check_connected(),
      check_value(targetdeclination >= -90.0f && targetdeclination <= +90.0f)
    );
  }

  return_t<float> priv_get_targetrightascension() const {
    return visit(
      [this]() {
        return get_targetrightascension();
      },
      check_connected()
    );
  }

  return_t<void> priv_put_targetrightascension(float targetrightascension) {
    return visit(
      [this, targetrightascension]() {
        return put_targetrightascension(targetrightascension);
      },
      check_connected(),
      check_value(targetrightascension >= 0.0f && targetrightascension <= +24.0f)
    );
  }

  return_t<bool> priv_get_tracking() const {
    return visit(
      [this]() {
        return get_tracking();
      },
      check_connected()
    );
  }

  return_t<void> priv_put_tracking(bool tracking) {
    return visit(
      [this, tracking]() {
        return put_tracking(tracking);
      },
      check_connected()
    );
  }

  return_t<driver_rate_t> priv_get_trackingrate() const {
    return visit(
      [this]() {
        return get_trackingrate();
      },
      check_connected()
    );
  }

  return_t<void> priv_put_trackingrate(int rate) {
    return visit(
      [this, rate]() {
        return put_trackingrate(static_cast<driver_rate_t>(rate));
      },
      check_connected(),
      check_value(rate >= 0 && rate <= 3)
    );
  }

  return_t<std::string> priv_get_utcdate() const {
    return visit(
      [this]() {
        return get_utcdate();
      },
      check_connected()
    );
  }

  return_t<void> priv_put_utcdate(const std::string& utc) {
    return visit(
      [this, utc]() {
        return put_utcdate(utc);
      },
      check_connected()
    );
  }

  // operations
  return_t<void> priv_abortslew() {
    return visit(
      [this]() {
        return abortslew();
      },
      check_connected()
    );
  }

  return_t<void> priv_findhome() {
    return visit(
      [this]() {
        return findhome();
      },
      check_connected(),
      check_flag(get_canfindhome())
    );
  }

  return_t<void> priv_moveaxis(int axis, float rate) {
    return visit(
      [this, axis, rate]() {
        return moveaxis(axis, rate);
      },
      check_connected(),
      check_axis(axis),
      check_flag(get_canmoveaxis(axis)),
      check_value(rate > -9.0f && rate < +9.0f)
    );
  }

  return_t<void> priv_park() {
    return visit(
      [this]() {
        return park();
      },
      check_connected(),
      check_flag(get_canpark())
    );
  }

  return_t<void> priv_pulseguide(int direction, int duration) {
    return visit(
      [this, direction, duration]() {
        return pulseguide(direction, duration);
      },
      check_connected(),
      check_flag(get_canpulseguide())
    );
  }

  return_t<void> priv_setpark() {
    return visit(
      [this]() {
        return setpark();
      },
      check_connected(),
      check_flag(get_cansetpark())
    );
  }

  return_t<void> priv_slewtoaltaz(float altitude, float azimuth) {
    return visit(
      [this, altitude, azimuth]() {
        return slewtoaltaz(altitude, azimuth);
      },
      check_connected(),
      check_flag(get_canslewaltaz())
    );
  }

  return_t<void> priv_slewtoaltazasync(float altitude, float azimuth) {
    return visit(
      [this, altitude, azimuth]() {
        return slewtoaltazasync(altitude, azimuth);
      },
      check_connected(),
      check_flag(get_canslewaltazasync()),
      check_value(azimuth >= 0.0f && azimuth <= 360.f),
      check_value(altitude >= -90.0f && altitude <= +90.f)
    );
  }

  return_t<void> priv_slewtocoordinates(
    float rightascension, float declination) {
    return visit(
      [this, rightascension, declination]() {
        return slewtocoordinates(rightascension, declination);
      },
      check_connected(),
      check_flag(get_canslew())
    );
  }

  return_t<void> priv_slewtocoordinatesasync(
    float rightascension, float declination) {
    return visit(
      [this, rightascension, declination]() {
        return slewtocoordinatesasync(rightascension, declination);
      },
      check_connected(),
      check_flag(get_canslewasync()),
      check_value(declination >= -90.0f && declination <= +90.0f),
      check_value(rightascension >= 0.0f && rightascension <= +24.0f)
    );
  }

  return_t<void> priv_slewtotarget() {
    return visit(
      [this]() {
        return slewtotarget();
      },
      check_connected(),
      check_flag(get_canslew())
    );
  }

  return_t<void> priv_slewtotargetasync() {
    return visit(
      [this]() {
        return slewtotargetasync();
      },
      check_connected(),
      check_flag(get_canslewasync())
    );
  }

  return_t<void> priv_synctoaltaz(float altitude, float azimuth) {
    return visit(
      [this, altitude, azimuth]() {
        return synctoaltaz(altitude, azimuth);
      },
      check_connected(),
      check_flag(get_cansyncaltaz()),
      check_value(azimuth >= 0.0f && azimuth <= 360.f),
      check_value(altitude >= -90.0f && altitude <= +90.f)
    );
  }

  return_t<void> priv_synctocoordinates(
    float rightascension, float declination) {
    return visit(
      [this, rightascension, declination]() {
        return synctocoordinates(rightascension, declination);
      },
      check_connected(),
      check_flag(get_cansync()),
      check_value(declination >= -90.0f && declination <= +90.0f),
      check_value(rightascension >= 0.0f && rightascension <= +24.0f)
    );
  }

  return_t<void> priv_synctotarget() {
    return visit(
      [this]() {
        return synctotarget();
      },
      check_connected(),
      check_parked(),
      check_flag(get_cansync())
    );
  }

  return_t<void> priv_unpark() {
    return visit(
      [this]() {
        return unpark();
      },
      check_connected(),
      check_flag(get_canunpark())
    );
  }

 public:
  telescope(const telescopeinfo_t& telescopeinfo)
  : device()
  , telescopeinfo(telescopeinfo)
  { }

  virtual ~telescope()
  { }

  // read-only properties
  virtual return_t<float> get_altitude() const {
    return not_implemented();
  }
  virtual return_t<float> get_azimuth() const {
    return not_implemented();
  }
  virtual return_t<float> get_declination() const {
    return not_implemented();
  }
  virtual return_t<float> get_rightascension() const {
    return not_implemented();
  }
  virtual return_t<bool> get_athome() const {
    return not_implemented();
  }
  virtual return_t<bool> get_atpark() const {
    return not_implemented();
  }
  virtual return_t<bool> get_ispulseguiding() const {
    return not_implemented();
  }
  virtual return_t<bool> get_slewing() const {
    return not_implemented();
  }
  virtual return_t<float> get_siderealtime() const {
    return not_implemented();
  }
  virtual return_t<destination_side_of_pier_t> get_destinationsideofpier(float, float) const {
    return not_implemented();
  }

  // constants
  virtual return_t<std::string> get_description() const { return telescopeinfo.description; }
  virtual return_t<std::string> get_driverinfo() const { return telescopeinfo.driverinfo; }
  virtual return_t<std::string> get_driverversion() const { return telescopeinfo.driverversion; }
  virtual return_t<int> get_interfaceversion() const { return telescopeinfo.interfaceversion; }
  virtual return_t<std::string> get_name() const { return telescopeinfo.name; }
  virtual return_t<std::vector<std::string>> get_supportedactions() const {
    return std::vector<std::string> { };
  }
  virtual return_t<alignment_mode_t> get_alignmentmode() const { return telescopeinfo.alignmentmode; }
  virtual return_t<float> get_aperturearea() const { return telescopeinfo.aperturearea; }
  virtual return_t<float> get_aperturediameter() const { return telescopeinfo.aperturediameter; }
  virtual return_t<float> get_focallength() const { return telescopeinfo.focallength; }
  virtual return_t<equatorial_system_t> get_equatorialsystem() const {
    return telescopeinfo.equatorialsystem;
  }
  virtual return_t<std::vector<axis_rate_t>> get_axisrates(int axis) const {
    return check_axis(axis).map([&]() {
      return telescopeinfo.axisrates;
    });
  }
  virtual return_t<std::vector<driver_rate_t>> get_trackingrates() const {
    return telescopeinfo.trackingrates;
  }

  // read-wrie properties
  virtual return_t<float> get_declinationrate() const {
    return not_implemented();
  }
  virtual return_t<void> put_declinationrate(float) {
    return not_implemented();
  }
  virtual return_t<float> get_rightascensionrate() const {
    return not_implemented();
  }
  virtual return_t<void> put_rightascensionrate(float) {
    return not_implemented();
  }
  virtual return_t<bool> get_doesrefraction() const {
    return not_implemented();
  }
  virtual return_t<void> put_doesrefraction(bool) {
    return not_implemented();
  }
  virtual return_t<float> get_guideratedeclination() const {
    return not_implemented();
  }
  virtual return_t<void> put_guideratedeclination(float) {
    return not_implemented();
  }
  virtual return_t<float> get_guideraterightascension() const {
    return not_implemented();
  }
  virtual return_t<void> put_guideraterightascension(float) {
    return not_implemented();
  }
  virtual return_t<int> get_sideofpier() const {
    return not_implemented();
  }
  virtual return_t<void> put_sideofpier(int) {
    return not_implemented();
  }
  virtual return_t<float> get_siteelevation() const {
    return not_implemented();
  }
  virtual return_t<void> put_siteelevation(float) {
    return not_implemented();
  }
  virtual return_t<float> get_sitelatitude() const {
    return not_implemented();
  }
  virtual return_t<void> put_sitelatitude(float) {
    return not_implemented();
  }
  virtual return_t<float> get_sitelongitude() const {
    return not_implemented();
  }
  virtual return_t<void> put_sitelongitude(float) {
    return not_implemented();
  }
  virtual return_t<int> get_slewsettletime() const {
    return not_implemented();
  }
  virtual return_t<void> put_slewsettletime(int) {
    return not_implemented();
  }
  virtual return_t<float> get_targetdeclination() const {
    return not_implemented();
  }
  virtual return_t<void> put_targetdeclination(float) {
    return not_implemented();
  }
  virtual return_t<float> get_targetrightascension() const {
    return not_implemented();
  }
  virtual return_t<void> put_targetrightascension(float) {
    return not_implemented();
  }
  virtual return_t<bool> get_tracking() const {
    return not_implemented();
  }
  virtual return_t<void> put_tracking(bool) {
    return not_implemented();
  }
  virtual return_t<driver_rate_t> get_trackingrate() const {
    return driver_rate_t::sidereal;
  }
  virtual return_t<void> put_trackingrate(driver_rate_t) {
    return not_implemented();
  }

  virtual return_t<std::string> get_utcdate() const {
    utcdate_t utcdate;

    get_utctm(&utcdate);
    return utcdate.format_utc();
  }

  virtual return_t<void> put_utcdate(const std::string& utc) {
    return utcdate_t::parse_utc(utc)
      .flat_map([this](auto&& parsed) {
        return put_utctm(parsed);
      });
  }

  virtual return_t<void> get_utctm(utcdate_t*) const {
    return not_implemented();
  }

  virtual return_t<void> put_utctm(utcdate_t) {
    return not_implemented();
  }

  // flags
  virtual return_t<bool> get_canfindhome() const {
    return (telescopeinfo.flags & telescope_flags_t::can_find_home) != 0;
  }
  virtual return_t<bool> get_canmoveaxis(int axis) const {
    return (telescopeinfo.flags & (telescope_flags_t::can_move_axis_0 << axis)) != 0;
  }
  virtual return_t<bool> get_canpark() const {
    return (telescopeinfo.flags & telescope_flags_t::can_park) != 0;
  }
  virtual return_t<bool> get_canpulseguide() const {
    return (telescopeinfo.flags & telescope_flags_t::can_pulse_guide) != 0;
  }
  virtual return_t<bool> get_cansetdeclinationrate() const {
    return (telescopeinfo.flags & telescope_flags_t::can_set_declination_rate) != 0;
  }
  virtual return_t<bool> get_cansetguiderates() const {
    return (telescopeinfo.flags & telescope_flags_t::can_set_guide_rates) != 0;
  }
  virtual return_t<bool> get_cansetpark() const {
    return (telescopeinfo.flags & telescope_flags_t::can_set_park) != 0;
  }
  virtual return_t<bool> get_cansetpierside() const {
    return (telescopeinfo.flags & telescope_flags_t::can_set_pier_side) != 0;
  }
  virtual return_t<bool> get_cansetrightascensionrate() const {
    return (telescopeinfo.flags & telescope_flags_t::can_set_right_ascension_rate) != 0;
  }
  virtual return_t<bool> get_cansettracking() const {
    return (telescopeinfo.flags & telescope_flags_t::can_set_tracking) != 0;
  }
  virtual return_t<bool> get_canslew() const {
    return (telescopeinfo.flags & telescope_flags_t::can_slew) != 0;
  }
  virtual return_t<bool> get_canslewaltaz() const {
    return (telescopeinfo.flags & telescope_flags_t::can_slew_altaz) != 0;
  }
  virtual return_t<bool> get_canslewaltazasync() const {
    return (telescopeinfo.flags & telescope_flags_t::can_slew_altaz_async) != 0;
  }
  virtual return_t<bool> get_canslewasync() const {
    return (telescopeinfo.flags & telescope_flags_t::can_slew_async) != 0;
  }
  virtual return_t<bool> get_cansync() const {
    return (telescopeinfo.flags & telescope_flags_t::can_sync) != 0;
  }
  virtual return_t<bool> get_cansyncaltaz() const {
    return (telescopeinfo.flags & telescope_flags_t::can_sync_altaz) != 0;
  }
  virtual return_t<bool> get_canunpark() const {
    return (telescopeinfo.flags & telescope_flags_t::can_unpark) != 0;
  }

  // operations
  virtual return_t<void> abortslew() {
    return not_implemented();
  }
  virtual return_t<void> findhome() {
    return not_implemented();
  }
  virtual return_t<void> moveaxis(int, float) {
    return not_implemented();
  }
  virtual return_t<void> park() {
    return not_implemented();
  }
  virtual return_t<void> pulseguide(int, int) {
    return not_implemented();
  }
  virtual return_t<void> setpark() {
    return not_implemented();
  }
  virtual return_t<void> slewtoaltaz(float, float) {
    return not_implemented();
  }
  virtual return_t<void> slewtoaltazasync(float, float) {
    return not_implemented();
  }
  virtual return_t<void> slewtocoordinates(float, float) {
    return not_implemented();
  }
  virtual return_t<void> slewtocoordinatesasync(float, float) {
    return not_implemented();
  }
  virtual return_t<void> slewtotarget() {
    return not_implemented();
  }
  virtual return_t<void> slewtotargetasync() {
    return not_implemented();
  }
  virtual return_t<void> synctoaltaz(float, float) {
    return not_implemented();
  }
  virtual return_t<void> synctocoordinates(float, float) {
    return not_implemented();
  }
  virtual return_t<void> synctotarget() {
    return not_implemented();
  }
  virtual return_t<void> unpark() {
    return not_implemented();
  }
};

class telescope_resource : public device_resource<telescope> {
 public:
  telescope_resource()
  : device_resource("telescope") {
    // read-only properties
    define_get("altitude", [](const telescope* tel, const arguments_t&) {
      return tel->priv_get_altitude();
    });
    define_get("azimuth", [](const telescope* tel, const arguments_t&) {
      return tel->priv_get_azimuth();
    });
    define_get("declination", [](const telescope* tel, const arguments_t&) {
      return tel->priv_get_declination();
    });
    define_get("rightascension", [](const telescope* tel, const arguments_t&) {
      return tel->priv_get_rightascension();
    });
    define_get("athome", [](const telescope* tel, const arguments_t&) {
      return tel->priv_get_athome();
    });
    define_get("atpark", [](const telescope* tel, const arguments_t&) {
      return tel->priv_get_atpark();
    });
    define_get("ispulseguiding", [](const telescope* tel, const arguments_t&) {
      return tel->priv_get_ispulseguiding();
    });
    define_get("slewing", [](const telescope* tel, const arguments_t&) {
      return tel->priv_get_slewing();
    });
    define_get("siderealtime", [](const telescope* tel, const arguments_t&) {
      return tel->priv_get_siderealtime();
    });
    define_get("destinationsideofpier", [](const telescope* tel, const arguments_t& args) {
      return coord_t::parse(args).flat_map([tel](const coord_t& coord) {
        return tel->priv_get_destinationsideofpier(coord.rightascension, coord.declination)
          .map([](destination_side_of_pier_t d) {
            return static_cast<int>(d);
          });
      });
    });

    // constants
    define_get("alignmentmode", [](const telescope* tel, const arguments_t&) {
      return tel->get_alignmentmode()
        .map([](alignment_mode_t alignmentmode) {
          return static_cast<int>(alignmentmode);
        });
    });
    define_get("aperturearea", [](const telescope* tel, const arguments_t&) {
      return tel->get_aperturearea();
    });
    define_get("aperturediameter", [](const telescope* tel, const arguments_t&) {
      return tel->get_aperturediameter();
    });
    define_get("focallength", [](const telescope* tel, const arguments_t&) {
      return tel->get_focallength();
    });
    define_get("equatorialsystem", [](const telescope* tel, const arguments_t&) {
      return tel->get_equatorialsystem()
        .map([](equatorial_system_t equatorialsystem) {
          return static_cast<int>(equatorialsystem);
        });
    });
    define_get("axisrates", [](const telescope* tel, const arguments_t& args) {
      return parser::parser_t::parse<int>(args, fields::axis_f)
        .flat_map([tel](int axis) {
          return tel->get_axisrates(axis).map([](auto&& axisrates) {
            json_value out_axisrates;

            std::transform(
              std::cbegin(axisrates),
              std::cend(axisrates),
              std::back_inserter(out_axisrates),
              [](auto&& r) {
                return json_value {
                  {"Minimum", r.minimum},
                  {"Maximum", r.maximum}
                };
              }
            );

            return out_axisrates;
          });
        });
    });
    define_get("trackingrates", [](const telescope* tel, const arguments_t&) {
      return tel->get_trackingrates()
        .map([](auto&& trackingrates) {
          json_value out_trackingrates;

          std::transform(
            std::cbegin(trackingrates),
            std::cend(trackingrates),
            std::back_inserter(out_trackingrates),
            [](auto&& rate) {
              return static_cast<int>(rate);
            }
          );

          return out_trackingrates;
        });
    });

    // flags
    define_get("canfindhome", [](const telescope* tel, const arguments_t&) {
      return tel->get_canfindhome();
    });
    define_get("canmoveaxis", [](const telescope* tel, const arguments_t& args) {
      return parser::parser_t::parse<int>(args, fields::axis_f)
        .flat_map([tel](int axis) {
          return tel->check_axis(axis).flat_map([tel, axis]() {
            return tel->get_canmoveaxis(axis);
          });
        });
    });
    define_get("canpark", [](const telescope* tel, const arguments_t&) {
      return tel->get_canpark();
    });
    define_get("canpulseguide", [](const telescope* tel, const arguments_t&) {
      return tel->get_canpulseguide();
    });
    define_get("cansetdeclinationrate", [](const telescope* tel, const arguments_t&) {
      return tel->get_cansetdeclinationrate();
    });
    define_get("cansetguiderates", [](const telescope* tel, const arguments_t&) {
      return tel->get_cansetguiderates();
    });
    define_get("cansetpark", [](const telescope* tel, const arguments_t&) {
      return tel->get_cansetpark();
    });
    define_get("cansetpierside", [](const telescope* tel, const arguments_t&) {
      return tel->get_cansetpierside();
    });
    define_get("cansetrightascensionrate", [](const telescope* tel, const arguments_t&) {
      return tel->get_cansetrightascensionrate();
    });
    define_get("cansettracking", [](const telescope* tel, const arguments_t&) {
      return tel->get_cansettracking();
    });
    define_get("canslew", [](const telescope* tel, const arguments_t&) {
      return tel->get_canslew();
    });
    define_get("canslewaltaz", [](const telescope* tel, const arguments_t&) {
      return tel->get_canslewaltaz();
    });
    define_get("canslewaltazasync", [](const telescope* tel, const arguments_t&) {
      return tel->get_canslewaltazasync();
    });
    define_get("canslewasync", [](const telescope* tel, const arguments_t&) {
      return tel->get_canslewasync();
    });
    define_get("cansync", [](const telescope* tel, const arguments_t&) {
      return tel->get_cansync();
    });
    define_get("cansyncaltaz", [](const telescope* tel, const arguments_t&) {
      return tel->get_cansyncaltaz();
    });
    define_get("canunpark", [](const telescope* tel, const arguments_t&) {
      return tel->get_canunpark();
    });

    // read-wrie properties
    define_ops(
      "declinationrate",
      [](const telescope* tel, const arguments_t&) {
        return tel->priv_get_declinationrate();
      },
      [](telescope* tel, const arguments_t& args) {
        return parser::parser_t::parse<float>(args, fields::declinationrate_f)
          .flat_map([tel](float declinationrate) {
            return tel->priv_put_declinationrate(declinationrate);
          });
      });
    define_ops(
      "doesrefraction",
      [](const telescope* tel, const arguments_t&) {
        return tel->priv_get_doesrefraction();
      },
      [](telescope* tel, const arguments_t& args) {
        return parser::parser_t::parse<bool>(args, fields::doesrefraction_f)
          .flat_map([=](bool doesrefraction) {
            return tel->priv_put_doesrefraction(doesrefraction);
          });
      });
    define_ops(
      "guideratedeclination",
      [](const telescope* tel, const arguments_t&) {
        return tel->priv_get_guideratedeclination();
      },
      [](telescope* tel, const arguments_t& args) {
        return parser::parser_t::parse<float>(args, fields::guideratedeclination_f)
          .flat_map([=](float guideratedeclination) {
            return tel->priv_put_guideratedeclination(guideratedeclination);
          });
      });
    define_ops(
      "guideraterightascension",
      [](const telescope* tel, const arguments_t&) {
        return tel->priv_get_guideraterightascension();
      },
      [](telescope* tel, const arguments_t& args) {
        return parser::parser_t::parse<float>(args, fields::guideraterightascension_f)
          .flat_map([=](float guideraterightascension) {
            return tel->priv_put_guideraterightascension(guideraterightascension);
          });
      });
    define_ops(
      "rightascensionrate",
      [](const telescope* tel, const arguments_t&) {
        return tel->priv_get_rightascensionrate();
      },
      [](telescope* tel, const arguments_t& args) {
        return parser::parser_t::parse<float>(args, fields::rightascensionrate_f)
          .flat_map([=](float rightascensionrate) {
            return tel->priv_put_rightascensionrate(rightascensionrate);
          });
      });
    define_ops(
      "sideofpier",
      [](const telescope* tel, const arguments_t&) {
        return tel->priv_get_sideofpier();
      },
      [](telescope* tel, const arguments_t& args) {
        return parser::parser_t::parse<int>(args, fields::sideofpier_f)
          .flat_map([=](int sideofpier) {
            return tel->priv_put_sideofpier(sideofpier);
          });
      });
    define_ops(
      "siteelevation",
      [](const telescope* tel, const arguments_t&) {
        return tel->priv_get_siteelevation();
      },
      [](telescope* tel, const arguments_t& args) {
        return parser::parser_t::parse<float>(args, fields::siteelevation_f)
          .flat_map([=](float siteelevation) {
            return tel->priv_put_siteelevation(siteelevation);
          });
      });
    define_ops(
      "sitelatitude",
      [](const telescope* tel, const arguments_t&) {
        return tel->priv_get_sitelatitude();
      },
      [](telescope* tel, const arguments_t& args) {
        return parser::parser_t::parse<float>(args, fields::sitelatitude_f)
          .flat_map([=](float sitelatitude) {
            return tel->priv_put_sitelatitude(sitelatitude);
          });
      });
    define_ops(
      "sitelongitude",
      [](const telescope* tel, const arguments_t&) {
        return tel->priv_get_sitelongitude();
      },
      [](telescope* tel, const arguments_t& args) {
        return parser::parser_t::parse<float>(args, fields::sitelongitude_f)
          .flat_map([=](float sitelongitude) {
            return tel->priv_put_sitelongitude(sitelongitude);
          });
      });
    define_ops(
      "slewsettletime",
      [](const telescope* tel, const arguments_t&) {
        return tel->priv_get_slewsettletime();
      },
      [](telescope* tel, const arguments_t& args) {
        return parser::parser_t::parse<int>(args, fields::slewsettletime_f)
          .flat_map([=](int slewsettletime) {
            return tel->priv_put_slewsettletime(slewsettletime);
          });
      });
    define_ops(
      "targetdeclination",
      [](const telescope* tel, const arguments_t&) {
        return tel->priv_get_targetdeclination();
      },
      [](telescope* tel, const arguments_t& args) {
        return parser::parser_t::parse<float>(args, fields::targetdeclination_f)
          .flat_map([=](float targetdeclination) {
            return tel->priv_put_targetdeclination(targetdeclination);
          });
      });
    define_ops(
      "targetrightascension",
      [](const telescope* tel, const arguments_t&) {
        return tel->priv_get_targetrightascension();
      },
      [](telescope* tel, const arguments_t& args) {
        return parser::parser_t::parse<float>(args, fields::targetrightascension_f)
          .flat_map([=](float targetrightascension) {
            return tel->priv_put_targetrightascension(targetrightascension);
          });
      });
    define_ops(
      "tracking",
      [](const telescope* tel, const arguments_t&) {
        return tel->priv_get_tracking();
      },
      [](telescope* tel, const arguments_t& args) {
        return parser::parser_t::parse<bool>(args, fields::tracking_f)
          .flat_map([=](bool tracking) {
            return tel->priv_put_tracking(tracking);
          });
      });
    define_ops(
      "trackingrate",
      [](const telescope* tel, const arguments_t&) {
        return tel->priv_get_trackingrate()
          .map([](driver_rate_t t) {
            return static_cast<int>(t);
          });
      },
      [](telescope* tel, const arguments_t& args) {
        return parser::parser_t::parse<int>(args, fields::trackingrate_f)
          .flat_map([=](int trackingrate) {
            return tel->priv_put_trackingrate(trackingrate);
          });
      });
    define_ops(
      "utcdate",
      [](const telescope* tel, const arguments_t&) {
        return tel->priv_get_utcdate();
      },
      [](telescope* tel, const arguments_t& args) {
        return parser::parser_t::parse<std::string_view>(args, fields::utcdate_f)
          .flat_map([=](std::string_view utc) {
            return tel->priv_put_utcdate(std::string{utc});
          });
      });

    // operations
    define_put("abortslew", [](telescope* tel, const arguments_t&) {
      return tel->priv_abortslew();
    });
    define_put("findhome", [](telescope* tel, const arguments_t&) {
      return tel->priv_findhome();
    });
    define_put("setpark", [](telescope* tel, const arguments_t&) {
      return tel->priv_setpark();
    });
    define_put("park", [](telescope* tel, const arguments_t&) {
      return tel->priv_park();
    });
    define_put("slewtotarget", [](telescope* tel, const arguments_t&) {
      return tel->priv_slewtotarget();
    });
    define_put("slewtotargetasync", [](telescope* tel, const arguments_t&) {
      return tel->priv_slewtotargetasync();
    });
    define_put("synctotarget", [](telescope* tel, const arguments_t&) {
      return tel->priv_synctotarget();
    });
    define_put("unpark", [](telescope* tel, const arguments_t&) {
      return tel->priv_unpark();
    });
    define_put("moveaxis", [](telescope* tel, const arguments_t& args) {
      return move_t::parse(args)
        .flat_map([=](const move_t& move) {
          return tel->priv_moveaxis(move.axis, move.rate);
        });
    });
    define_put("pulseguide", [](telescope* tel, const arguments_t& args) {
      return pulse_t::parse(args)
        .flat_map([=](const pulse_t& pulse) {
          return tel->priv_pulseguide(pulse.direction, pulse.duration);
        });
    });
    define_put("slewtoaltaz", [](telescope* tel, const arguments_t& args) {
      return altazm_t::parse(args)
        .flat_map([=](const altazm_t& altazm) {
          return tel->priv_slewtoaltaz(altazm.altitude, altazm.azimuth);
        });
    });
    define_put("slewtoaltazasync", [](telescope* tel, const arguments_t& args) {
      return altazm_t::parse(args)
        .flat_map([=](const altazm_t& altazm) {
          return tel->priv_slewtoaltazasync(altazm.altitude, altazm.azimuth);
        });
    });
    define_put("slewtocoordinates", [](telescope* tel, const arguments_t& args) {
      return coord_t::parse(args)
        .flat_map([=](const coord_t& coord) {
          return tel->priv_slewtocoordinates(coord.rightascension, coord.declination);
        });
    });
    define_put("slewtocoordinatesasync", [](telescope* tel, const arguments_t& args) {
      return coord_t::parse(args)
        .flat_map([=](const coord_t& coord) {
          return tel->priv_slewtocoordinatesasync(coord.rightascension, coord.declination);
        });
    });
    define_put("synctoaltaz", [](telescope* tel, const arguments_t& args) {
      return altazm_t::parse(args)
        .flat_map([=](const altazm_t& altazm) {
          return tel->priv_synctoaltaz(altazm.altitude, altazm.azimuth);
        });
    });
    define_put("synctocoordinates", [](telescope* tel, const arguments_t& args) {
      return coord_t::parse(args)
        .flat_map([=](const coord_t& coord) {
          return tel->priv_synctocoordinates(coord.rightascension, coord.declination);
        });
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

    json_value obj = {
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
