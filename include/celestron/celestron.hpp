// Copyright (C) 2023 Marrony Neris

#ifndef INCLUDE_CELESTRON_CELESTRON_HPP_
#define INCLUDE_CELESTRON_CELESTRON_HPP_

#include <memory>
#include <string>
#include <cmath>
#include <cstdint>

#include <telescope.hpp>
#include <serial.hpp>
#include <time.hpp>
#include <astronomy.hpp>

namespace celestron {

enum class tracking_mode_t : char {
  off      = 0,
  alt_azm  = 1,
  eq_north = 2,
  eq_south = 3
};

struct nexstar_protocol {
  virtual ~nexstar_protocol() { }

  virtual int send_command(
    const char* in, int in_size, char* out, int out_size) = 0;

  bool get_version(int* major, int* minor) {
    const char command[] = { 'V' };
    char output[3];

    int nbytes = send_command(command, 1, output, 3);

    if (nbytes != 3) return false;
    if (output[2] != '#') return false;

    *major = output[0];
    *minor = output[1];
    return true;
  }

  bool get_model(int* model) {
    const char command[] = { 'm' };
    char output[2];

    int nbytes = send_command(command, 1, output, 2);

    if (nbytes != 2) return false;
    if (output[1] != '#') return false;

    return true;
  }

  std::string get_model_string(int model) {
    if (model == 1) return "GPS Series";
    if (model == 3) return "i-Series";
    if (model == 4) return "i-Series SE";
    if (model == 5) return "CGE";
    if (model == 6) return "Advanced GT";
    if (model == 7) return "SLT";
    if (model == 9) return "CPC";
    if (model == 10) return "GT";
    if (model == 11) return "4/5 SE";
    if (model == 12) return "6/8 SE";
    if (model == 13) return "GCE Pro";
    if (model == 14) return "CGEM DX";
    if (model == 15) return "LCM";
    if (model == 16) return "Sky Prodigy";
    if (model == 17) return "CPC Deluxe";
    if (model == 18) return "GT 16";
    if (model == 19) return "StarSeeker";
    if (model == 20) return "Advanced VX";
    if (model == 21) return "Cosmos";
    if (model == 22) return "Evolution";
    if (model == 23) return "CGX";
    if (model == 24) return "CGXL";
    if (model == 25) return "Astrofi";
    if (model == 26) return "SkyWatcher";

    return "Unknown model";
  }

  // converts [0x0000, 0x0000ffff] to [0, 360]
  // converts [0x0000, 0xffffffff] to [0, 360]
  float nexstar_to_degree(std::uint32_t value, bool precise) {
    const float k1 = 360.0f / 0x100000000;
    const float k2 = 360.0f / 0x000010000;
    const float k = precise ? k1 : k2;

    return value * k;
  }

  // converts [0, 360] to [0x0000, 0x0000ffff]
  // converts [0, 360] to [0x0000, 0xffffffff]
  std::uint32_t degree_to_nexstar(float angle, bool precise) {
    const float k1 = 0x100000000 / 360.0f;
    const float k2 = 0x000010000 / 360.0f;
    const float k = precise ? k1 : k2;

    return static_cast<std::uint32_t>(std::fmod(angle, 360.0f) * k);
  }

  // converts [0, 360] to [-90, +90]
  float fix_declination(float angle) {
    // angle = angle - 360.0f * std::floor(angle / 360.0f);
    angle = std::fmod(angle, 360.0f);

    if (angle < 0) angle += 360.0f;

    if (angle > 90.0 && angle <= 270.0f)
      return 180.0f - angle;

    if (angle > 270.0f && angle <= 360.0f)
      return angle - 360.0f;

    return angle;
  }

  bool get_ra_de(float* ra, float* de, bool precise) {
    const char command[] = { precise ? 'e' : 'E' };
    char output[19];
    std::uint32_t ra_int, de_int;

    int size = precise ? 18 : 10;

    int nbytes = send_command(command, 1, output, size);

    if (nbytes != size) return false;
    if (output[size - 1] != '#') return false;

    output[size] = '\0';

    if (std::sscanf(output, "%x,%x#", &ra_int, &de_int) != 2)
      return false;

    *ra = nexstar_to_degree(ra_int, precise) / 15.0f;
    *de = nexstar_to_degree(de_int, precise);

    return true;
  }

  bool goto_ra_de(float ra, float de, bool precise) {
    char command[19];
    char output[1];

    int size = precise ? 18 : 10;
    const char* fmt = precise ? "r%08X,%08X" : "R%04X,%04X";

    if (de < 0.0f)
      de += 360.0f;

    std::uint32_t ra_int = degree_to_nexstar(ra * 15.0f, precise);
    std::uint32_t de_int = degree_to_nexstar(de, precise);

    std::snprintf(command, size + 1, fmt, ra_int, de_int);

    int nbytes = send_command(command, size, output, 1);

    if (nbytes != 1) return false;
    if (output[0] != '#') return false;

    return true;
  }

  bool get_azm_alt(float* azm, float* alt, bool precise) {
    const char command[] = { precise ? 'z' : 'Z' };
    char output[19];
    std::uint32_t alt_int, azm_int;

    int size = precise ? 18 : 10;

    int nbytes = send_command(command, 1, output, size);

    if (nbytes != size) return false;
    if (output[size - 1] != '#') return false;

    output[size] = '\0';

    if (std::sscanf(output, "%x,%x#", &azm_int, &alt_int) != 2)
      return false;

    *azm = nexstar_to_degree(azm_int, precise);
    *alt = nexstar_to_degree(alt_int, precise);

    return true;
  }

  bool is_goto_in_progress(bool* is_inprogress) {
    const char command[] = { 'L' };
    char output[2];

    int nbytes = send_command(command, 1, output, 2);

    if (nbytes != 2) return false;
    if (output[1] != '#') return false;

    *is_inprogress = output[0] == '1';

    return true;
  }

  //bool get_time() {}
  //bool set_time() {}

  bool get_location(
    float* latitude,
    float* longitude) {
    const char command[] = { 'w' };
    char output[9];

    int nbytes = send_command(command, 1, output, 9);

    if (nbytes != 9) return false;
    if (output[8] != '#') return false;

    alpaca::astronomy::dms_t lat{
      output[0], output[1], output[2]
    };

    alpaca::astronomy::dms_t lon{
      output[4], output[5], output[6]
    };

    *latitude = output[3] == 1
                ? -lat.to_decimal()
                : +lat.to_decimal();

    *longitude = output[7] == 1
                ? -lon.to_decimal()
                : +lon.to_decimal();

    return true;
  }

  bool set_location(
    float latitude,
    float longitude) {
    alpaca::astronomy::dms_t lat{ latitude };
    alpaca::astronomy::dms_t lon{ longitude };

    const char command[] = {
      'W',
      static_cast<char>(std::abs(lat.degree) & 0xff),
      static_cast<char>(lat.minute & 0xff),
      static_cast<char>(lat.second & 0xff),
      static_cast<char>(latitude >= 0 ? 0 : 1),
      static_cast<char>(std::abs(lon.degree) & 0xff),
      static_cast<char>(lon.minute & 0xff),
      static_cast<char>(lon.second & 0xff),
      static_cast<char>(longitude >= 0 ? 0 : 1)
    };
    char output[1];

    int nbytes = send_command(command, 9, output, 1);

    if (nbytes != 1) return false;
    if (output[0] != '#') return false;

    return true;
  }

  bool slew_variable(int axis, float rate) {
    int rate_abs = static_cast<int>(std::abs(rate * 60 * 60 * 4)); //arcseconds/second
    char direction = rate >= 0 ? 6 : 7;
    char axis_op = axis == 0 ? 16 : 17;

    const char command[] = {
      'P',
      3,
      axis_op,
      direction,
      static_cast<char>((rate_abs >> 8) & 0xff),
      static_cast<char>(rate_abs & 0xff),
      0,
      0,
    };
    char output[1];

    int nbytes = send_command(command, 8, output, 1);

    if (nbytes != 1) return false;
    if (output[0] != '#') return false;

    return true;
  }

  bool get_tracking_mode(tracking_mode_t* mode) {
    const char command[] = { 't' };
    char output[2];

    int nbytes = send_command(command, 1, output, 2);

    if (nbytes != 2) return false;
    if (output[1] != '#') return false;

    *mode = static_cast<tracking_mode_t>(output[0]);
    return true;
  }

  bool set_tracking_mode(tracking_mode_t mode) {
    const char command[] = { 'T', static_cast<char>(mode) };
    char output[1];

    int nbytes = send_command(command, 2, output, 1);

    if (nbytes != 1) return false;
    if (output[0] != '#') return false;

    return true;
  }

  bool is_aligned(bool* aligned) {
    const char command[] = { 'J' };
    char output[2];

    int nbytes = send_command(command, 1, output, 2);

    if (nbytes != 2) return false;
    if (output[1] != '#') return false;

    *aligned = output[0] == 1;
    return true;
  }

  bool cancel_goto() {
    const char command[] = { 'M' };
    char output[1];

    int nbytes = send_command(command, 1, output, 1);

    if (nbytes != 1) return false;
    if (output[0] != '#') return false;

    return true;
  }

  bool echo(char ch) {
    const char command[] = { 'K', ch };
    char output[2];

    int nbytes = send_command(command, 1, output, 2);

    if (nbytes != 2) return false;
    if (output[0] != ch) return false;
    if (output[1] != '#') return false;

    return true;
  }
};

struct simulator_protocol : nexstar_protocol {
  float target_rightascension = 0;
  float target_declination = 0;
  float rightascension = 0;
  float declination = 0;

  float latitude;
  float longitude;

  bool is_tracking = false;
  float slew_rate[2] = {0, 0};

  enum class state_kind : int {
    no_op,
    slewing,
    moving
  };

  state_kind state = state_kind::no_op;

  alpaca::utcdate_t last_ts = alpaca::utcdate_t::now();

  auto step(float target, float* actual, float delta_time, int axis) -> void {
    switch (state) {
      case state_kind::no_op:
        break;

      case state_kind::slewing:
      {
        float diff = target - *actual;
        float dist = std::sqrt(diff * diff);

        if (dist <= 0.1f) {
          *actual = target;
        } else {
          float rate = 1.0f;

          if (dist <= 5.0f)
            rate = 0.25f;
          else if (dist <= 10.0f)
            rate = 0.50f;
          else if (dist <= 20.0f)
            rate = 0.75f;

          *actual += std::min(diff * rate, 9.0f) * delta_time;
        }
        break;
      }

      case state_kind::moving:
      {
        *actual += slew_rate[axis] * delta_time;
        break;
      }
    }
  }

  auto step() -> void {
    alpaca::utcdate_t now = alpaca::utcdate_t::now();
    float delta_time = (now - last_ts) / 1000000.0f;
    last_ts = now;

    switch (state) {
      case state_kind::no_op:
        break;

      case state_kind::slewing:
      {
        step(target_rightascension, &rightascension, delta_time, 0);
        step(target_declination, &declination, delta_time, 1);

        printf("step(%f) => (%f %f) => (%f %f)\n",
          delta_time, rightascension, declination, target_rightascension, target_declination);

        bool is_slewing = target_rightascension != rightascension || target_declination != declination;
        if (!is_slewing)
          state = state_kind::no_op;

        break;
      }

      case state_kind::moving:
        rightascension += slew_rate[0] * delta_time;
        declination += slew_rate[1] * delta_time;
        break;
    }
  }

  virtual int send_command(
    const char* in, int in_size, char* out, int out_size) {

    step();

    switch (in[0]) {
      case 'V':
        out[0] = 1;
        out[1] = 2;
        out[2] = '#';
        return 2;

      case 'm':
        out[0] = 20;
        out[1] = '#';
        return 2;

      case 'w':
      {
        alpaca::astronomy::dms_t lat{ latitude };
        alpaca::astronomy::dms_t lon{ longitude };

        out[0] = static_cast<char>(std::abs(lat.degree));
        out[1] = static_cast<char>(lat.minute);
        out[2] = static_cast<char>(lat.second);
        out[3] = static_cast<char>(latitude >= 0 ? 0 : 1);
        out[4] = static_cast<char>(std::abs(lon.degree));
        out[5] = static_cast<char>(lon.minute);
        out[6] = static_cast<char>(lon.second);
        out[7] = static_cast<char>(longitude >= 0 ? 0 : 1);
        out[8] = '#';
        return 9;
      }

      case 'W':
      {
        alpaca::astronomy::dms_t lat{ in[1], in[2], in[3] };
        alpaca::astronomy::dms_t lon{ in[5], in[6], in[7] };

        latitude = in[4] == 1 ? -lat.to_decimal() : +lat.to_decimal();
        longitude = in[8] == 1 ? -lon.to_decimal() : +lon.to_decimal();

        out[0] = '#';
        return 1;
      }

      case 'E':
      {
        printf("E: %f %f\n", rightascension, declination);
        std::snprintf(out, out_size + 1, "%04X,%04X#",
          degree_to_nexstar(rightascension, false),
          degree_to_nexstar(declination, false));
        return 10;
      }

      case 'e':
      {
        std::snprintf(out, out_size + 1, "%08X,%08X#",
          degree_to_nexstar(rightascension, true),
          degree_to_nexstar(declination, true));
        return 18;
      }

      case 'Z':
      {
        float azimuth, altitude;

        alpaca::astronomy::ra_de_to_azm_alt(
          alpaca::utcdate_t::now(),
          rightascension, declination,
          latitude, longitude,
          &azimuth, &altitude
        );

        std::snprintf(out, out_size + 1, "%04X,%04X#",
          degree_to_nexstar(azimuth, false),
          degree_to_nexstar(altitude, false));
        return 10;
      }

      case 'z':
      {
        float azimuth, altitude;

        alpaca::astronomy::ra_de_to_azm_alt(
          alpaca::utcdate_t::now(),
          rightascension, declination,
          latitude, longitude,
          &azimuth, &altitude
        );

        std::snprintf(out, out_size + 1, "%08X,%08X#",
          degree_to_nexstar(azimuth, true),
          degree_to_nexstar(altitude, true));
        return 18;
      }

      case 's':
        out[0] = '#';
        return 1;

      case 'S':
        out[0] = '#';
        return 1;

      case 't':
        out[0] = is_tracking
                   ? static_cast<char>(tracking_mode_t::eq_north)
                   : static_cast<char>(tracking_mode_t::off);
        out[1] = '#';
        return 2;

      case 'T':
        is_tracking = in[1] != static_cast<char>(tracking_mode_t::off);
        out[0] = '#';
        return 1;

      case 'J':
        out[0] = 1;
        out[1] = '#';
        return 2;

      case 'L':
      {
        bool is_slewing = state != state_kind::no_op;
        out[0] = is_slewing ? '1' : '0';
        out[1] = '#';
        return 2;
      }

      case 'M':
        state = state_kind::no_op;
        target_rightascension = rightascension;
        target_declination = declination;
        out[0] = '#';
        return 1;

      case 'r':
      case 'R':
      {
        uint32_t ra = -1, de = -1;

        std::sscanf(in, "%*c%x,%x", &ra, &de);

        bool precise = in[0] == 'r';

        printf("%s %d %d\n", in, ra, de);
        target_rightascension = nexstar_to_degree(ra, precise);
        target_declination = nexstar_to_degree(de, precise);
        printf("r %f %f\n", target_rightascension, target_declination);
        state = state_kind::slewing;

        out[0] = '#';
        return 1;
      }

      case 'b':
      case 'B':
      {
        uint32_t azm, alt;

        std::sscanf(in, "%*c%x,%x", &azm, &alt);

        bool precise = in[0] == 'b';

        float azimuth = nexstar_to_degree(azm, precise);
        float altitude = nexstar_to_degree(alt, precise);

        alpaca::astronomy::azm_alt_to_ra_de(
          alpaca::utcdate_t::now(),
          azimuth, altitude,
          latitude, longitude,
          &rightascension, &declination);

        out[0] = '#';
        return 1;
      }

      case 'P':
        switch (in[1]) {
          case 3:
          {
            std::cout << in[0] << ' ';
            std::cout << (int)(unsigned char)in[1] << ' '; //3
            std::cout << (int)(unsigned char)in[2] << ' '; //axis 16, 17
            std::cout << (int)(unsigned char)in[3] << ' '; //dir 6, 7
            std::cout << (int)(unsigned char)in[4] << ' '; //hi
            std::cout << (int)(unsigned char)in[5] << ' '; //lo
            std::cout << (int)(unsigned char)in[6] << ' '; //0
            std::cout << (int)(unsigned char)in[7] << std::endl; //0

            std::uint32_t rate_int = static_cast<std::uint8_t>(in[4]) << 8 | static_cast<std::uint8_t>(in[5]);
            float rate = rate_int / (3600.0f * 4);
            int axis = in[2] - 16;

            if (in[3] == 7) rate *= -1;

            std::cout << "Rate: " << rate_int << std::endl;
            std::cout << "Rate: " << rate << std::endl;
            std::cout << "Axis: " << axis << std::endl;

            slew_rate[axis] = rate;

            state = rate_int != 0 ? state_kind::moving : state_kind::no_op;

            out[0] = '#';
            return 1;
          }
        }
        break;
    }

    return 0;
  }
};

struct serial_protocol : nexstar_protocol {
  alpaca::serial serial;
  std::string_view port;
  int baudRate;

  virtual int send_command(
    const char* in, int in_size, char* out, int out_size) {

    if (!serial.is_open()) {
      if (!serial.open(port, baudRate))
        return -1;
    }

    if (serial.write(in, in_size) != in_size)
      return -1;

    return serial.read(out, out_size);
  }

  serial_protocol(std::string_view port, int baudRate)
  : serial(), port(port), baudRate(baudRate) { }

  virtual ~serial_protocol() {
    if (serial.is_open())
      serial.close();
  }
};

#if 0
const siderealRate = 361 / (24 * 60 * 60);

return [
  {
    Minimum: 2 * siderealRate,
    Maximum: 2 * siderealRate,
  },
  {
    Minimum: 4 * siderealRate,
    Maximum: 4 * siderealRate,
  },
  {
    Minimum: 8 * siderealRate,
    Maximum: 8 * siderealRate,
  },
  {
    Minimum: 16 * siderealRate,
    Maximum: 16 * siderealRate,
  },
  {
    Minimum: 32 * siderealRate,
    Maximum: 32 * siderealRate,
  },
  {
    Minimum: 0.3,
    Maximum: 0.3,
  },
  {
    Minimum: 1,
    Maximum: 1,
  },
  {
    Minimum: 2,
    Maximum: 2,
  },
  {
    Minimum: 4,
    Maximum: 4,
  },
#endif

class celestron_telescope : public alpaca::telescope {
  std::shared_ptr<nexstar_protocol> protocol;

 public:
  celestron_telescope(
    const alpaca::telescopeinfo_t& info,
    const std::shared_ptr<nexstar_protocol>& protocol)
  : alpaca::telescope(info)
  , protocol(protocol)
  { }

  // device
  virtual alpaca::deviceinfo_t get_deviceinfo() const {
    int model = 0;

    check_op(protocol->get_model(&model));

    return {
      .name = protocol->get_model_string(model),
      .device_type = "telescope",
      .device_number = device_number,
      .unique_id = "fb9472c8-6217-4140-9ebe-67d9ca0754c1"
    };
  }

  virtual void put_connected(bool connected) {
    if (is_connected && connected) return;
    if (!is_connected && !connected) return;

    //if (connected) {
    //  protocol = create_protocol();
    //} else {
    //  protocol = nullptr;
    //}

    is_connected = connected;
  }

  virtual bool get_connected() const {
    return is_connected;
  }

  // telescope

  // read-only properties
  virtual float get_altitude() const {
    check_connected();

    float azm, alt;

    check_op(protocol->get_azm_alt(&azm, &alt, false));

    return alt;
  }

  virtual float get_azimuth() const {
    check_connected();

    float azm, alt;

    check_op(protocol->get_azm_alt(&azm, &alt, false));

    return azm;
  }

  virtual float get_declination() const {
    check_connected();

    float ra, de;

    check_op(protocol->get_ra_de(&ra, &de, false));

    return de;
  }

  virtual float get_rightascension() const {
    check_connected();

    float ra, de;

    check_op(protocol->get_ra_de(&ra, &de, false));

    return ra;
  }
  virtual bool get_athome() const {
    check_connected();
    return false;
   }
  virtual bool get_atpark() const {
    check_connected();
    return false;
  }
  virtual bool get_ispulseguiding() const {
    check_connected();
    check_flag(get_canpulseguide());
    return false;
  }
  virtual bool get_slewing() const {
    check_connected();

    bool is_slewing;
    check_op(protocol->is_goto_in_progress(&is_slewing));

    return is_slewing;
  }

  virtual float get_siderealtime() const {
    check_connected();

    float latitude, longitude;

    check_op(protocol->get_location(&latitude, &longitude));

    return alpaca::to_lst(alpaca::utcdate_t::now(), longitude) / 15.0f;
  }

  virtual alpaca::destination_side_of_pier_t get_destinationsideofpier(
    float rightascension, float declination) const {
    return alpaca::destination_side_of_pier_t::pier_unknown;
  }

  // read-wrie properties
  //virtual bool get_doesrefraction() const { return 0; }
  //virtual void put_doesrefraction(bool) {}

  virtual float get_guideratedeclination() const {
    check_connected();
    return 0;
  }
  virtual void put_guideratedeclination(float) {
    check_connected();
    check_flag(get_cansetguiderates());
  }

  virtual float get_guideraterightascension() const {
    check_connected();
    return 0;
  }
  virtual void put_guideraterightascension(float) {
    check_connected();
    check_flag(get_cansetguiderates());
  }

  virtual int get_sideofpier() const { return 0; }
  virtual void put_sideofpier(int) {}

  virtual float get_sitelatitude() const {
    check_connected();

    float latitude, longitude;

    check_op(protocol->get_location(&latitude, &longitude));

    return latitude;
  }

  virtual void put_sitelatitude(float angle) {
    check_connected();
    check_value(angle >= -90.0f && angle <= +90.0f);

    float latitude, longitude;

    check_op(protocol->get_location(&latitude, &longitude));
    check_op(protocol->set_location(angle, longitude));
  }

  virtual float get_sitelongitude() const {
    check_connected();

    float latitude, longitude;

    check_op(protocol->get_location(&latitude, &longitude));

    return longitude;
  }

  virtual void put_sitelongitude(float angle) {
    check_connected();
    check_value(angle >= -180.0f && angle <= +180.0f);

    float latitude, longitude;

    check_op(protocol->get_location(&latitude, &longitude));
    check_op(protocol->set_location(latitude, angle));
  }

  virtual int get_slewsettletime() const {
    check_connected();
    not_implemented();
    return 0;
  }

  virtual void put_slewsettletime(int) {
    check_connected();
    not_implemented();
  }

  float targetdeclination = 100;
  float targetrightascension = 100;
  virtual float get_targetdeclination() const {
    check_connected();
    check_set(targetdeclination < 100);
    return targetdeclination;
  }

  virtual void put_targetdeclination(float targetdeclination) {
    check_connected();
    check_value(targetdeclination >= -90.0f && targetdeclination <= +90.0f);
    this->targetdeclination = targetdeclination;
  }

  virtual float get_targetrightascension() const {
    check_connected();
    check_set(targetrightascension < 100);
    return targetrightascension;
  }

  virtual void put_targetrightascension(float targetrightascension) {
    check_connected();
    check_value(targetrightascension >= 0.0f && targetrightascension <= +24.0f);
    this->targetrightascension = targetrightascension;
  }

  virtual bool get_tracking() const {
    check_connected();

    tracking_mode_t mode;
    check_op(protocol->get_tracking_mode(&mode));

    return mode != tracking_mode_t::off;
  }

  virtual void put_tracking(bool tracking) {
    check_connected();

    tracking_mode_t mode = tracking ? tracking_mode_t::eq_north : tracking_mode_t::off;

    check_op(protocol->set_tracking_mode(mode));
  }

  virtual alpaca::driver_rate_t get_trackingrate() const { return alpaca::driver_rate_t::sidereal; }
  virtual void put_trackingrate(alpaca::driver_rate_t) {}

  std::string utc = "2022-12-04T17:45:31.1234567Z";
  virtual std::string get_utcdate() const {
    return utc;
  }

  virtual void put_utcdate(const std::string& utc) {
    this->utc = utc;

    int year, month, day;
    int hour, minute, seconds, milliseconds;

    int count = std::sscanf(
      utc.c_str(),
      // 2022-12-04T17:45:31.1234567Z
      "%04d-%02d-%02dT%02d:%02d:%02d.%dZ",
      &year, &month, &day, &hour, &minute,
      &seconds, &milliseconds);

    if (count != 7) {
      throw alpaca::error::invalid_value();
    }
  }

  // operations
  virtual void abortslew() {
    check_connected();

    check_op(protocol->cancel_goto());
  }

  //virtual void findhome() {
  //  check_connected();
  //}

  virtual void moveaxis(int axis, float rate) {
    alpaca::telescope::moveaxis(axis, rate);

    check_op(protocol->slew_variable(axis, rate));
  }

  //virtual void park() {
  //  check_connected();
  //  check_flag(get_canpark());
  //}

  virtual void pulseguide(int direction, int duration) {
    check_connected();
    check_flag(get_canpulseguide());
  }

  virtual void setpark() {
    check_connected();
    check_flag(get_canpark());
  }

  virtual void slewtoaltaz(float altitude, float azimuth) {
    throw alpaca::error::not_implemented();
  }

  virtual void slewtoaltazasync(float altitude, float azimuth) {
    check_connected();
    check_flag(get_canslewaltazasync());
    check_value(azimuth >= 0.0f && azimuth <= 360.f);
    check_value(altitude >= -90.0f && altitude <= +90.f);
  }

  virtual void slewtocoordinates(float rightascension, float declination) {
    throw alpaca::error::not_implemented();
  }

  virtual void slewtocoordinatesasync(float rightascension, float declination) {
    check_connected();
    check_flag(get_canslewasync());
    check_value(declination >= -90.0f && declination <= +90.0f);
    check_value(rightascension >= 0.0f && rightascension <= +24.0f);

    targetrightascension = rightascension;
    targetdeclination = declination;
    check_op(protocol->goto_ra_de(rightascension, declination, false));
  }

  virtual void slewtotarget() {
    throw alpaca::error::not_implemented();
  }

  virtual void slewtotargetasync() {
    check_connected();
    check_flag(get_canslewasync());

    check_op(protocol->goto_ra_de(targetrightascension, targetdeclination, false));
  }

  virtual void synctoaltaz(float altitude, float azimuth) {
    check_connected();
    check_flag(get_cansyncaltaz());
    check_value(azimuth >= 0.0f && azimuth <= 360.f);
    check_value(altitude >= -90.0f && altitude <= +90.f);
  }

  virtual void synctocoordinates(float rightascension, float declination) {
    check_connected();
    check_flag(get_cansync());
    check_value(declination >= -90.0f && declination <= +90.0f);
    check_value(rightascension >= 0.0f && rightascension <= +24.0f);

    targetrightascension = rightascension;
    targetdeclination = declination;

    check_op(protocol->goto_ra_de(rightascension, declination, false));
  }

  virtual void synctotarget() {
    check_connected();
    check_parked();
    check_flag(get_cansync());

    //protocol->goto_ra_de(target_ra, target_de);
  }

  virtual void unpark() {
    check_connected();

    if(!get_canunpark())
      throw alpaca::error::not_implemented();
  }
};

}  // namespace celestron

#endif  // INCLUDE_CELESTRON_CELESTRON_HPP_
