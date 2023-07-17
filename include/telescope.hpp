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
  telescopeinfo_t telescopeinfo;
  bool is_connected;

 protected:
  inline void check_connected() const {
    if (!is_connected) throw error::not_connected();
  }

  inline void check_parked() const {
    if (priv_get_atpark()) throw error::parked();
  }

  inline void check_flag(bool flag) const {
    if (!flag) throw error::not_implemented();
  }

  [[noreturn]] inline void not_implemented() const {
    throw error::not_implemented();
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

 private:
  // resource needs to be able to call telescope private methods
  friend class telescope_resource;

  // read-only properties
  float priv_get_altitude() const {
    check_connected();
    return get_altitude();
  }

  float priv_get_azimuth() const {
    check_connected();
    return get_azimuth();
  }

  float priv_get_declination() const {
    check_connected();
    return get_declination();
  }

  float priv_get_rightascension() const {
    check_connected();
    return get_rightascension();
  }

  bool priv_get_athome() const {
    check_connected();
    return get_athome();
  }

  bool priv_get_atpark() const {
    check_connected();
    return get_atpark();
  }

  bool priv_get_ispulseguiding() const {
    check_connected();
    check_flag(get_canpulseguide());
    return get_ispulseguiding();
  }

  bool priv_get_slewing() const {
    check_connected();
    return get_slewing();
  }

  float priv_get_siderealtime() const {
    check_connected();
    return get_siderealtime();
  }

  destination_side_of_pier_t priv_get_destinationsideofpier(
    float rightascension, float declination) const {
    check_connected();
    return get_destinationsideofpier(rightascension, declination);
  }

  // read-wrie properties
  float priv_get_declinationrate() const {
    check_connected();
    return get_declinationrate();
  }

  void priv_put_declinationrate(float declinationrate) {
    check_connected();
    check_flag(get_cansetdeclinationrate());

    put_declinationrate(declinationrate);
  }

  float priv_get_rightascensionrate() const {
    check_connected();
    return get_rightascensionrate();;
  }

  void priv_put_rightascensionrate(float rightascensionrate) {
    check_connected();
    check_flag(get_cansetrightascensionrate());

    put_rightascensionrate(rightascensionrate);
  }

  bool priv_get_doesrefraction() const {
    check_connected();
    return get_doesrefraction();
  }

  void priv_put_doesrefraction(bool doesrefraction) {
    check_connected();
    put_doesrefraction(doesrefraction);
  }

  float priv_get_guideratedeclination() const {
    check_connected();
    return get_guideratedeclination();
  }

  void priv_put_guideratedeclination(float guideratedeclination) {
    check_connected();
    check_flag(get_cansetguiderates());
    put_guideratedeclination(guideratedeclination);
  }

  float priv_get_guideraterightascension() const {
    check_connected();
    return get_guideraterightascension();
  }

  void priv_put_guideraterightascension(float guideraterightascension) {
    check_connected();
    put_guideraterightascension(guideraterightascension);
  }

  int priv_get_sideofpier() const {
    check_connected();
    return get_sideofpier();
  }

  void priv_put_sideofpier(int sideofpier) {
    check_connected();
    put_sideofpier(sideofpier);
  }


  float priv_get_siteelevation() const {
    check_connected();
    return get_siteelevation();
  }

  void priv_put_siteelevation(float elevation) {
    check_connected();
    check_value(elevation >= -300.0f && elevation <= 10000.0f);
    put_siteelevation(elevation);
  }

  float priv_get_sitelatitude() const {
    check_connected();
    return get_sitelatitude();
  }

  void priv_put_sitelatitude(float latitude) {
    check_connected();
    check_value(latitude >= -90.0f && latitude <= +90.0f);

    put_sitelatitude(latitude);
  }

  float priv_get_sitelongitude() const {
    check_connected();
    return get_sitelongitude();
  }

  void priv_put_sitelongitude(float angle) {
    check_connected();
    check_value(angle >= -180.0f && angle <= +180.0f);
    put_sitelongitude(angle);
  }


  int priv_get_slewsettletime() const {
    check_connected();
    return get_slewsettletime();
  }

  void priv_put_slewsettletime(int slewsettletime) {
    check_connected();
    put_slewsettletime(slewsettletime);
  }

  float priv_get_targetdeclination() const {
    check_connected();
    return get_targetdeclination();
  }

  void priv_put_targetdeclination(float targetdeclination) {
    check_connected();
    check_value(targetdeclination >= -90.0f && targetdeclination <= +90.0f);
    put_targetdeclination(targetdeclination);
  }

  float priv_get_targetrightascension() const {
    check_connected();
    return get_targetrightascension();
  }

  void priv_put_targetrightascension(float targetrightascension) {
    check_connected();
    check_value(targetrightascension >= 0.0f && targetrightascension <= +24.0f);
    put_targetrightascension(targetrightascension);
  }

  bool priv_get_tracking() const {
    check_connected();
    return get_tracking();
  }

  void priv_put_tracking(bool tracking) {
    check_connected();
    put_tracking(tracking);
  }

  driver_rate_t priv_get_trackingrate() const {
    check_connected();
    return get_trackingrate();
  }

  void priv_put_trackingrate(int rate) {
    check_connected();
    check_value(rate >= 0 && rate <= 3);
    put_trackingrate(static_cast<driver_rate_t>(rate));
  }

  std::string priv_get_utcdate() const {
    check_connected();
    return get_utcdate();
  }

  void priv_put_utcdate(const std::string& utc) {
    check_connected();
    put_utcdate(utc);
  }

  // operations
  void priv_abortslew() {
    check_connected();

    abortslew();
  }

  void priv_findhome() {
    check_connected();
    check_flag(get_canfindhome());

    findhome();
  }

  void priv_moveaxis(int axis, float rate) {
    check_connected();
    check_flag(get_canmoveaxis(axis));
    check_value(rate > -9.0f && rate < +9.0f);

    moveaxis(axis, rate);
  }

  void priv_park() {
    check_connected();
    check_flag(get_canpark());

    park();
  }

  void priv_pulseguide(int direction, int duration) {
    check_connected();
    check_flag(get_canpulseguide());

    pulseguide(direction, duration);
  }

  void priv_setpark() {
    check_connected();
    check_flag(get_cansetpark());

    setpark();
  }

  void priv_slewtoaltaz(float altitude, float azimuth) {
    check_connected();
    check_flag(get_canslewaltaz());

    slewtoaltaz(altitude, azimuth);
  }

  void priv_slewtoaltazasync(float altitude, float azimuth) {
    check_connected();
    check_flag(get_canslewaltazasync());
    check_value(azimuth >= 0.0f && azimuth <= 360.f);
    check_value(altitude >= -90.0f && altitude <= +90.f);

    slewtoaltazasync(altitude, azimuth);
  }

  void priv_slewtocoordinates(
    float rightascension, float declination) {
    check_connected();

    slewtocoordinates(rightascension, declination);
  }

  void priv_slewtocoordinatesasync(
    float rightascension, float declination) {
    check_connected();
    check_flag(get_canslewasync());
    check_value(declination >= -90.0f && declination <= +90.0f);
    check_value(rightascension >= 0.0f && rightascension <= +24.0f);

    slewtocoordinatesasync(rightascension, declination);
  }

  void priv_slewtotarget() {
    check_connected();
    check_flag(get_canslewasync());

    slewtotarget();
  }

  void priv_slewtotargetasync() {
    check_connected();
    check_flag(get_canslewasync());

    slewtotargetasync();
  }

  void priv_synctoaltaz(float altitude, float azimuth) {
    check_connected();
    check_flag(get_cansyncaltaz());
    check_value(azimuth >= 0.0f && azimuth <= 360.f);
    check_value(altitude >= -90.0f && altitude <= +90.f);

    synctoaltaz(altitude, azimuth);
  }

  void priv_synctocoordinates(
    float rightascension, float declination) {
    check_connected();
    check_flag(get_cansync());
    check_value(declination >= -90.0f && declination <= +90.0f);
    check_value(rightascension >= 0.0f && rightascension <= +24.0f);

    synctocoordinates(rightascension, declination);
  }

  void priv_synctotarget() {
    check_connected();
    check_parked();
    check_flag(get_cansync());

    synctotarget();
  }

  void priv_unpark() {
    check_connected();
    check_flag(get_canunpark());

    unpark();
  }

 public:
  telescope(const telescopeinfo_t& telescopeinfo)
  : device()
  , telescopeinfo(telescopeinfo)
  , is_connected(false)
  { }

  virtual ~telescope()
  { }

  virtual void put_connected(bool connected) {
    if (is_connected && connected) return;
    if (!is_connected && !connected) return;
    is_connected = connected;
  }

  virtual bool get_connected() const {
    return is_connected;
  }

  // read-only properties
  virtual float get_altitude() const { not_implemented(); }
  virtual float get_azimuth() const { not_implemented(); }
  virtual float get_declination() const { not_implemented(); }
  virtual float get_rightascension() const { not_implemented(); }
  virtual bool get_athome() const { not_implemented(); }
  virtual bool get_atpark() const { not_implemented(); }
  virtual bool get_ispulseguiding() const { not_implemented(); }
  virtual bool get_slewing() const { not_implemented(); }
  virtual float get_siderealtime() const { not_implemented(); }
  virtual destination_side_of_pier_t get_destinationsideofpier(float rightascension, float declination) const { not_implemented(); }

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
  virtual void put_declinationrate(float) { not_implemented(); }
  virtual float get_rightascensionrate() const { return 0; }
  virtual void put_rightascensionrate(float) { not_implemented(); }
  virtual bool get_doesrefraction() const { not_implemented(); }
  virtual void put_doesrefraction(bool) { not_implemented(); }
  float get_guideratedeclination() const { not_implemented(); }
  virtual void put_guideratedeclination(float) { not_implemented(); }
  virtual float get_guideraterightascension() const { not_implemented(); }
  virtual void put_guideraterightascension(float) { not_implemented(); }
  virtual int get_sideofpier() const { not_implemented(); }
  virtual void put_sideofpier(int) { not_implemented(); }
  virtual float get_siteelevation() const { not_implemented(); }
  virtual void put_siteelevation(float elevation) { not_implemented(); }
  virtual float get_sitelatitude() const { not_implemented(); }
  virtual void put_sitelatitude(float) { not_implemented(); }
  virtual float get_sitelongitude() const { not_implemented(); }
  virtual void put_sitelongitude(float) { not_implemented(); }
  virtual int get_slewsettletime() const { not_implemented(); }
  virtual void put_slewsettletime(int) { not_implemented(); }
  virtual float get_targetdeclination() const { not_implemented(); }
  virtual void put_targetdeclination(float) { not_implemented(); }
  virtual float get_targetrightascension() const { not_implemented(); }
  virtual void put_targetrightascension(float) { not_implemented(); }
  virtual bool get_tracking() const { not_implemented(); }
  virtual void put_tracking(bool) { not_implemented(); }
  virtual driver_rate_t get_trackingrate() const { not_implemented(); }
  virtual void put_trackingrate(driver_rate_t rate) { not_implemented(); }

  virtual std::string get_utcdate() const {
    utcdate_t utcdate;

    get_utctm(&utcdate);
    return utcdate.format_utc();
  }

  virtual void put_utcdate(const std::string& utc) {
    put_utctm(utcdate_t::parse_utc(utc));
  }

  virtual void get_utctm(utcdate_t*) const { not_implemented(); }
  virtual void put_utctm(utcdate_t) { not_implemented(); }

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
  virtual void abortslew() { not_implemented(); }
  virtual void findhome() { not_implemented(); }
  virtual void moveaxis(int axis, float rate) { not_implemented(); }
  virtual void park() { not_implemented(); }
  virtual void pulseguide(int direction, int duration) { not_implemented(); }
  virtual void setpark() { not_implemented(); }
  virtual void slewtoaltaz(float altitude, float azimuth) { not_implemented(); }
  virtual void slewtoaltazasync(float altitude, float azimuth) { not_implemented(); }
  virtual void slewtocoordinates(float rightascension, float declination) { not_implemented(); }
  virtual void slewtocoordinatesasync(float rightascension, float declination) { not_implemented(); }
  virtual void slewtotarget() { not_implemented(); }
  virtual void slewtotargetasync() { not_implemented(); }
  virtual void synctoaltaz(float altitude, float azimuth) { not_implemented(); }
  virtual void synctocoordinates(float rightascension, float declination) { not_implemented(); }
  virtual void synctotarget() { not_implemented(); }
  virtual void unpark() { not_implemented(); }
};

class telescope_resource : public device_resource<telescope> {
 public:
  telescope_resource()
  : device_resource("telescope") {
    // read-only properties
    define_get("altitude", [](const telescope* tel, const arguments_t& args) {
      return tel->priv_get_altitude();
    });
    define_get("azimuth", [](const telescope* tel, const arguments_t& args) {
      return tel->priv_get_azimuth();
    });
    define_get("declination", [](const telescope* tel, const arguments_t& args) {
      return tel->priv_get_declination();
    });
    define_get("rightascension", [](const telescope* tel, const arguments_t& args) {
      return tel->priv_get_rightascension();
    });
    define_get("athome", [](const telescope* tel, const arguments_t& args) {
      return tel->priv_get_athome();
    });
    define_get("atpark", [](const telescope* tel, const arguments_t& args) {
      return tel->priv_get_atpark();
    });
    define_get("ispulseguiding", [](const telescope* tel, const arguments_t& args) {
      return tel->priv_get_ispulseguiding();
    });
    define_get("slewing", [](const telescope* tel, const arguments_t& args) {
      return tel->priv_get_slewing();
    });
    define_get("siderealtime", [](const telescope* tel, const arguments_t& args) {
      return tel->priv_get_siderealtime();
    });
    define_get("destinationsideofpier", [](const telescope* tel, const arguments_t& args) {
      coord_t coord = coord_t::parse(args);
      return static_cast<int>(tel->priv_get_destinationsideofpier(coord.rightascension, coord.declination));
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
        return tel->priv_get_declinationrate();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->priv_put_declinationrate(
          parser::parser_t::parse<float>(args, fields::declinationrate_f));
      });
    define_ops(
      "doesrefraction",
      [](const telescope* tel, const arguments_t& args) {
        return tel->priv_get_doesrefraction();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->priv_put_doesrefraction(
          parser::parser_t::parse<bool>(args, fields::doesrefraction_f));
      });
    define_ops(
      "guideratedeclination",
      [](const telescope* tel, const arguments_t& args) {
        return tel->priv_get_guideratedeclination();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->priv_put_guideratedeclination(
          parser::parser_t::parse<float>(args, fields::guideratedeclination_f));
      });
    define_ops(
      "guideraterightascension",
      [](const telescope* tel, const arguments_t& args) {
        return tel->priv_get_guideraterightascension();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->priv_put_guideraterightascension(
          parser::parser_t::parse<float>(args, fields::guideraterightascension_f));
      });
    define_ops(
      "rightascensionrate",
      [](const telescope* tel, const arguments_t& args) {
        return tel->priv_get_rightascensionrate();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->priv_put_rightascensionrate(
          parser::parser_t::parse<float>(args, fields::rightascensionrate_f));
      });
    define_ops(
      "sideofpier",
      [](const telescope* tel, const arguments_t& args) {
        return tel->priv_get_sideofpier();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->priv_put_sideofpier(parser::parser_t::parse<int>(args, fields::sideofpier_f));
      });
    define_ops(
      "siteelevation",
      [](const telescope* tel, const arguments_t& args) {
        return tel->priv_get_siteelevation();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->priv_put_siteelevation(
          parser::parser_t::parse<float>(args, fields::siteelevation_f));
      });
    define_ops(
      "sitelatitude",
      [](const telescope* tel, const arguments_t& args) {
        return tel->priv_get_sitelatitude();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->priv_put_sitelatitude(
          parser::parser_t::parse<float>(args, fields::sitelatitude_f));
      });
    define_ops(
      "sitelongitude",
      [](const telescope* tel, const arguments_t& args) {
        return tel->priv_get_sitelongitude();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->priv_put_sitelongitude(
          parser::parser_t::parse<float>(args, fields::sitelongitude_f));
      });
    define_ops(
      "slewsettletime",
      [](const telescope* tel, const arguments_t& args) {
        return tel->priv_get_slewsettletime();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->priv_put_slewsettletime(
          parser::parser_t::parse<int>(args, fields::slewsettletime_f));
      });
    define_ops(
      "targetdeclination",
      [](const telescope* tel, const arguments_t& args) {
        return tel->priv_get_targetdeclination();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->priv_put_targetdeclination(
          parser::parser_t::parse<float>(args, fields::targetdeclination_f));
      });
    define_ops(
      "targetrightascension",
      [](const telescope* tel, const arguments_t& args) {
        return tel->priv_get_targetrightascension();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->priv_put_targetrightascension(
          parser::parser_t::parse<float>(args, fields::targetrightascension_f));
      });
    define_ops(
      "tracking",
      [](const telescope* tel, const arguments_t& args) {
        return tel->priv_get_tracking();
      },
      [](telescope* tel, const arguments_t& args) {
        tel->priv_put_tracking(parser::parser_t::parse<bool>(args, fields::tracking_f));
      });
    define_ops(
      "trackingrate",
      [](const telescope* tel, const arguments_t& args) {
        return static_cast<json_int>(tel->priv_get_trackingrate());
      },
      [](telescope* tel, const arguments_t& args) {
        tel->priv_put_trackingrate(
          parser::parser_t::parse<int>(args, fields::trackingrate_f));
      });
    define_ops(
      "utcdate",
      [](const telescope* tel, const arguments_t& args) {
        return tel->priv_get_utcdate();
      },
      [](telescope* tel, const arguments_t& args) {
        auto utc = parser::parser_t::parse<std::string>(args, fields::utcdate_f);
        tel->priv_put_utcdate(utc);
      });

    // operations
    define_put("abortslew", [](telescope* tel, const arguments_t& args) {
      tel->priv_abortslew();
    });
    define_put("findhome", [](telescope* tel, const arguments_t& args) {
      tel->priv_findhome();
    });
    define_put("setpark", [](telescope* tel, const arguments_t& args) {
      tel->priv_setpark();
    });
    define_put("park", [](telescope* tel, const arguments_t& args) {
      tel->priv_park();
    });
    define_put("slewtotarget", [](telescope* tel, const arguments_t& args) {
      tel->priv_slewtotarget();
    });
    define_put("slewtotargetasync", [](telescope* tel, const arguments_t& args) {
      tel->priv_slewtotargetasync();
    });
    define_put("synctotarget", [](telescope* tel, const arguments_t& args) {
      tel->priv_synctotarget();
    });
    define_put("unpark", [](telescope* tel, const arguments_t& args) {
      tel->priv_unpark();
    });
    define_put("moveaxis", [](telescope* tel, const arguments_t& args) {
      move_t move = move_t::parse(args);
      tel->priv_moveaxis(move.axis, move.rate);
    });
    define_put("pulseguide", [](telescope* tel, const arguments_t& args) {
      pulse_t pulse = pulse_t::parse(args);
      tel->priv_pulseguide(pulse.direction, pulse.duration);
    });
    define_put("slewtoaltaz", [](telescope* tel, const arguments_t& args) {
      altazm_t altazm = altazm_t::parse(args);
      tel->priv_slewtoaltaz(altazm.altitude, altazm.azimuth);
    });
    define_put("slewtoaltazasync", [](telescope* tel, const arguments_t& args) {
      altazm_t altazm = altazm_t::parse(args);
      tel->priv_slewtoaltazasync(altazm.altitude, altazm.azimuth);
    });
    define_put("slewtocoordinates", [](telescope* tel, const arguments_t& args) {
      coord_t coord = coord_t::parse(args);
      tel->priv_slewtocoordinates(coord.rightascension, coord.declination);
    });
    define_put("slewtocoordinatesasync", [](telescope* tel, const arguments_t& args) {
      coord_t coord = coord_t::parse(args);
      tel->priv_slewtocoordinatesasync(coord.rightascension, coord.declination);
    });
    define_put("synctoaltaz", [](telescope* tel, const arguments_t& args) {
      altazm_t altazm = altazm_t::parse(args);
      tel->priv_synctoaltaz(altazm.altitude, altazm.azimuth);
    });
    define_put("synctocoordinates", [](telescope* tel, const arguments_t& args) {
      coord_t coord = coord_t::parse(args);
      tel->priv_synctocoordinates(coord.rightascension, coord.declination);
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
