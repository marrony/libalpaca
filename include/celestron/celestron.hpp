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

enum class tracking_mode_kind : std::uint8_t {
  off      = 0,
  alt_azm  = 1,
  eq_north = 2,
  eq_south = 3
};

  //dev
  // 16 = azm/ra motor
  // 17 = alt/de motor
  // 176 = gps
  // 178 = rtc
enum class device_kind : std::uint8_t {
  azm_motor = 16,
  alt_motor = 17,
  gps       = 176,
  rtc       = 178
};

enum class passthrough_command_kind : std::uint8_t {
  slew_variable_positive = 6,
  slew_variable_negative = 7,
  slew_fixed_positive = 36,
  slew_fixed_negative = 37,

  // [ 'P', req_size, dev, cmd/arg0, arg1, arg2, arg3, resp_size ]

  // azm motor
  // + var azm    = 3, 16, 6, hi, lo, 0, 0
  // - var azm    = 3, 16, 7, hi, lo, 0, 0
  // + fix azm    = 2, 16, 36, rate, 0, 0, 0
  // - fix azm    = 2, 16, 37, rate, 0, 0, 0

  // alt motor
  // + var alt    = 3, 17, 6, hi, lo, 0, 0
  // - var alt    = 3, 17, 7, hi, lo, 0, 0
  // + fix azm    = 2, 17, 36, rate, 0, 0, 0
  // - fix azm    = 2, 17, 37, rate, 0, 0, 0

  //gps
  //is gps linked = 1, 176, 55, 0, 0, 0, 1
  //get latitude  = 1, 176, 1, 0, 0, 0, 3
  //get longitude = 1, 176, 2, 0, 0, 0, 3
  //get date      = 1, 176, 3, 0, 0, 0, 2
  //get year      = 1, 176, 4, 0, 0, 0, 2
  //get time      = 1, 176, 51, 0, 0, 0, 3

  //rtc
  //get date      = 1, 178, 3, 0, 0, 0, 2
  //get year      = 1, 178, 4, 0, 0, 0, 2
  //get time      = 1, 178, 51, 0, 0, 0, 3
  //set date      = 3, 178, 131, x, y, 0, 0
  //set year      = 3, 178, 132, x, y, 0, 0
  //set time      = 4, 178, 179, x, y, z, 0

  //misc
  //get dev ver   = 1, dev, 254, 0, 0, 0, 2 

};

struct void_t {};

template<typename T = void_t>
struct response_base_t {
  union {
    struct {
      T            payload;
      std::uint8_t always_0x23;
    };
    char data[sizeof(T) + sizeof(std::uint8_t)];
  };

  constexpr response_base_t() = default;

  constexpr auto is_ok() const -> bool {
    return always_0x23 == '#';
  }
};

template<>
struct response_base_t<void_t> {
  union {
    std::uint8_t always_0x23;
    char data[sizeof(std::uint8_t)];
  };

  constexpr response_base_t() = default;
};

template<typename T = void_t>
struct response_t : response_base_t<T> {

  constexpr response_t() = default;

  [[nodiscard]] constexpr auto is_ok() const -> bool {
    return response_base_t<T>::always_0x23 == '#';
  }

  template<typename U>
  struct is_convertible : std::integral_constant<bool, std::is_enum_v<U> || std::is_integral_v<U>>
  { };

  template<
    typename = std::enable_if<is_convertible<T>::value && !std::is_same_v<T, void_t>>
  >
  [[nodiscard]] constexpr auto parse(T* args) -> bool {
    *args = static_cast<T>(response_base_t<T>::payload);
    return true;
  }

  template<
    typename = std::enable_if<!is_convertible<T>::value && !std::is_same_v<T, void_t>>,
    typename... Args
  >
  [[nodiscard]] constexpr auto parse(Args... args) const -> bool {
    return response_base_t<T>::payload.parse(args...);
  }
};

template<std::uint8_t CMD, typename R>
struct get_command_t {
  union {
    std::uint8_t cmd;
    char data[sizeof(std::uint8_t)];
  };

  using response_t = response_t<R>;

  constexpr get_command_t()
  : cmd{CMD}
  { }
};

template<std::uint8_t CMD, typename T = void_t, typename R = void_t>
struct set_command_t {
  union {
    struct {
      std::uint8_t cmd;
      T            payload;
    };
    char data[sizeof(std::uint8_t) + sizeof(T)];
  };

  using response_t = response_t<R>;

  template<typename... Args>
  constexpr set_command_t(Args... args)
  : cmd{CMD}
  , payload{args...}
  { }

  template<typename... Args>
  [[nodiscard]] constexpr auto parse(Args... args) const -> bool {
    return payload.parse(args...);
  }
};

template<std::uint8_t CMD>
struct set_command_t<CMD, void_t, void_t> {
  union {
    std::uint8_t cmd;
    char data[sizeof(std::uint8_t)];
  };

  using response_t = response_t<void_t>;

  constexpr set_command_t()
  : cmd{CMD}
  { }

  [[nodiscard]] constexpr auto parse() const -> bool {
    return true;
  }
};

struct location_t {
  std::uint8_t latitude_degree;
  std::uint8_t latitude_minute;
  std::uint8_t latitude_second;
  std::uint8_t is_south;
  std::uint8_t longitude_degree;
  std::uint8_t longitude_minute;
  std::uint8_t longitude_second;
  std::uint8_t is_west;

  constexpr location_t() = default;

  constexpr location_t(float latitude, float longitude) {
    alpaca::astronomy::dms_t lat{ latitude };
    alpaca::astronomy::dms_t lon{ longitude };

    latitude_degree  = static_cast<std::uint8_t>(std::abs(lat.degree));
    latitude_minute  = static_cast<std::uint8_t>(lat.minute);
    latitude_second  = static_cast<std::uint8_t>(lat.second);
    is_south         = static_cast<std::uint8_t>(latitude >= 0 ? 0 : 1);
    longitude_degree = static_cast<std::uint8_t>(std::abs(lon.degree));
    longitude_minute = static_cast<std::uint8_t>(lon.minute);
    longitude_second = static_cast<std::uint8_t>(lon.second);
    is_west          = static_cast<std::uint8_t>(longitude >= 0 ? 0 : 1);
  }

  [[nodiscard]] constexpr auto parse(float* latitude, float* longitude) const -> bool {
    alpaca::astronomy::dms_t lat{ latitude_degree, latitude_minute, latitude_second };
    alpaca::astronomy::dms_t lon{ longitude_degree, longitude_minute, longitude_second };

    *latitude = is_south == 1 ? -lat.to_decimal() : +lat.to_decimal();
    *longitude = is_west == 1 ? -lon.to_decimal() : +lon.to_decimal();

    return true;
  }
};

struct utcdate_t {
  std::uint8_t hour;
  std::uint8_t minute;
  std::uint8_t second;
  std::uint8_t month;
  std::uint8_t day;
  std::uint8_t year;
  std::uint8_t offset;
  std::uint8_t isdst;

  constexpr utcdate_t() = default;

  constexpr utcdate_t(alpaca::utcdate_t utcdate, int offset_micros) {
    utcdate += offset_micros;

    std::tm local_tm;
    utcdate.to_local_tm(&local_tm);

    int gmt_offset = local_tm.tm_gmtoff/3600;

    if (gmt_offset < 0)
      gmt_offset += 256;

    hour   = static_cast<std::uint8_t>(local_tm.tm_hour); // hour (24 hour)
    minute = static_cast<std::uint8_t>(local_tm.tm_min); // minutes
    second = static_cast<std::uint8_t>(local_tm.tm_sec); // seconds
    month  = static_cast<std::uint8_t>(local_tm.tm_mon + 1); // month
    day    = static_cast<std::uint8_t>(local_tm.tm_mday); // day
    year   = static_cast<std::uint8_t>(local_tm.tm_year + 1900 - 2000); // year (century is 20)
    offset = static_cast<std::uint8_t>(gmt_offset); // gmt offset
    isdst  = static_cast<std::uint8_t>(local_tm.tm_isdst > 0 ? 1 : 0); // 1 dst, 0 std time
  }

  [[nodiscard]] constexpr auto parse(alpaca::utcdate_t* utcdate) const -> bool {
    int gmt_offset = offset;

    if (gmt_offset > 127)
      gmt_offset -= 256;

    std::tm local_tm;
    local_tm.tm_hour   = hour;
    local_tm.tm_min    = minute;
    local_tm.tm_sec    = second;
    local_tm.tm_mon    = month - 1;
    local_tm.tm_mday   = day;
    local_tm.tm_year   = year + 2000 - 1900;
    local_tm.tm_gmtoff = gmt_offset;
    local_tm.tm_isdst  = isdst;

    *utcdate = alpaca::utcdate_t::from_local_tm(&local_tm);

    return true;
  }
};

struct version_t {
  std::uint8_t major;
  std::uint8_t minor;

  constexpr version_t() = default;

  [[nodiscard]] constexpr auto parse(std::int32_t* major, std::int32_t* minor) const -> bool {
    *major = this->major;
    *minor = this->minor;
    return true;
  }
};

using get_location_t = get_command_t<'w', location_t>;
using set_location_t = set_command_t<'W', location_t>;

using get_utcdate_t = get_command_t<'h', utcdate_t>;
using set_utcdate_t = set_command_t<'H', utcdate_t>;


struct alignas(1) passthrough_command_t {
  // [ 'P', req_size, dev, cmd/arg0, arg1, arg2, arg3, resp_size ]
  std::uint8_t always_P;
  std::uint8_t request_arguments;
  device_kind device;
  passthrough_command_kind command;
  std::uint8_t args[3];
  std::uint8_t response_arguments;

  constexpr passthrough_command_t() = default;

  constexpr passthrough_command_t(device_kind device, passthrough_command_kind command, char arg0, char arg1, char arg2, char args_size, char response_size) {
    this->always_P = 'P';
    this->request_arguments = args_size + 1;
    this->device = device;
    this->command = command;
    this->args[0] = arg0;
    this->args[1] = arg1;
    this->args[2] = arg2;
    this->response_arguments = response_size;
  }
};

struct slew_variable_command_t {
  union {
    passthrough_command_t cmd;
    char data[8];
  };

  constexpr slew_variable_command_t(int axis, float rate) {
    int rate_abs = static_cast<int>(std::abs(rate * 3600 * 4)); //arcseconds/second

    passthrough_command_kind command = rate >= 0
        ? passthrough_command_kind::slew_variable_negative
        : passthrough_command_kind::slew_variable_positive;

    device_kind device = axis == 0
        ? device_kind::azm_motor
        : device_kind::alt_motor;

    char hi = static_cast<char>((rate_abs >> 8) & 0xff);
    char lo = static_cast<char>(rate_abs & 0xff);

    std::construct_at(&cmd, device, command, hi, lo, 0, 2, 0);
  }

  [[nodiscard]] constexpr auto parse(int* axis, float* rate) const -> bool {
    std::int32_t rate_int = cmd.args[0] << 8 | cmd.args[1];

    switch (cmd.device) {
      case device_kind::azm_motor:
        *axis = 0;
        break;

      case device_kind::alt_motor:
        *axis = 1;
        break;

      default:
        return false;
    }

    switch (cmd.command) {
      case passthrough_command_kind::slew_variable_positive:
        *rate = +rate_int / (3600.0f * 4);
        break;

      case passthrough_command_kind::slew_variable_negative:
        *rate = -rate_int / (3600.0f * 4);
        break;

      default:
        return false;
    }

    return true;
  }

};

std::ostream& operator<<(std::ostream& os, const passthrough_command_t& p) {
  std::cout << p.always_P << std::endl;
  std::cout << static_cast<int>(p.request_arguments) << std::endl;
  std::cout << static_cast<int>(p.device) << std::endl;
  std::cout << static_cast<int>(p.command) << std::endl;
  std::cout << static_cast<int>(p.args[0]) << std::endl;
  std::cout << static_cast<int>(p.args[1]) << std::endl;
  std::cout << static_cast<int>(p.args[2]) << std::endl;
  std::cout << static_cast<int>(p.response_arguments) << std::endl;
  return os;
}

struct nexstar_protocol {
  virtual ~nexstar_protocol() { }

  virtual int send_command(
    const void* in, int in_size, void* out, int out_size) = 0;

  bool get_version(std::int32_t* major, std::int32_t* minor) {
    using get_version_t = get_command_t<'V', version_t>;

    const get_version_t command;
    get_version_t::response_t response;

    int nbytes = send_command(command.data, sizeof(command), response.data, sizeof(response));

    if (nbytes != sizeof(response)) return false;
    if (!response.is_ok()) return false;

    return response.parse(major, minor);
  }

  bool get_model(std::int32_t* model) {
    using get_model_t = get_command_t<'m', std::int32_t>;

    const get_model_t command;
    get_model_t::response_t response;

    int nbytes = send_command(command.data, sizeof(command), response.data, sizeof(response));

    if (nbytes != sizeof(response)) return false;
    if (!response.is_ok()) return false;

    return response.parse(model);
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
    using is_goto_inprogress_t = get_command_t<'L', std::uint8_t>;

    const is_goto_inprogress_t command;
    is_goto_inprogress_t::response_t response;

    int nbytes = send_command(command.data, sizeof(command), response.data, sizeof(response));

    if (nbytes != sizeof(response)) return false;
    if (!response.is_ok()) return false;

    std::uint8_t value;
    if (response.parse(&value)) {
      *is_inprogress = value == '1';
      return true;
    }

    return false;
  }

  [[nodiscard]] bool get_utcdate(alpaca::utcdate_t* utcdate) {
    const get_utcdate_t command;
    get_utcdate_t::response_t response;

    int nbytes = send_command(command.data, sizeof(command), response.data, sizeof(response));

    if (nbytes != sizeof(response)) return false;
    if (!response.is_ok()) return false;

    return response.parse(utcdate);
  }

  [[nodiscard]] bool set_utcdate(alpaca::utcdate_t utcdate) {
    const set_utcdate_t command{utcdate, 0};
    set_utcdate_t::response_t response;

    int nbytes = send_command(command.data, sizeof(command), response.data, sizeof(response));

    if (nbytes != sizeof(response)) return false;
    if (!response.is_ok()) return false;

    return true;
  }

  [[nodiscard]] bool get_location(float* latitude, float* longitude) {
    const get_location_t command;
    get_location_t::response_t response;

    int nbytes = send_command(command.data, sizeof(command), response.data, sizeof(response));

    if (nbytes != sizeof(response)) return false;
    if (!response.is_ok()) return false;

    return response.parse(latitude, longitude);
  }

  [[nodiscard]] bool set_location(float latitude, float longitude) {
    const set_location_t command{latitude, longitude};
    set_location_t::response_t response;

    int nbytes = send_command(command.data, sizeof(command), response.data, sizeof(response));

    if (nbytes != sizeof(response)) return false;
    if (!response.is_ok()) return false;

    return true;
  }

  [[nodiscard]] bool slew_variable(int axis, float rate) {
    const slew_variable_command_t slew_command{axis, rate};
    response_t response;

    int nbytes = send_command(slew_command.data, sizeof(slew_command), response.data, sizeof(response));

    if (nbytes != sizeof(response)) return false;
    if (!response.is_ok()) return false;

    return true;
  }

  bool get_tracking_mode(tracking_mode_kind* mode) {
    using get_tracking_mode_t = get_command_t<'t', tracking_mode_kind>;

    const get_tracking_mode_t command;
    get_tracking_mode_t::response_t response;

    int nbytes = send_command(command.data, sizeof(command), response.data, sizeof(response));

    if (nbytes != sizeof(response)) return false;
    if (!response.is_ok()) return false;

    return response.parse(mode);
  }

  bool set_tracking_mode(tracking_mode_kind mode) {
    using set_tracking_mode_t = set_command_t<'T', tracking_mode_kind>;

    const set_tracking_mode_t command{mode};
    set_tracking_mode_t::response_t response;

    int nbytes = send_command(command.data, sizeof(command), response.data, sizeof(response));

    if (nbytes != sizeof(response)) return false;
    if (!response.is_ok()) return false;

    return true;
  }

  bool is_aligned(bool* aligned) {
    using is_aligned_t = get_command_t<'J', bool>;

    const is_aligned_t command;
    is_aligned_t::response_t response;

    int nbytes = send_command(command.data, sizeof(command), response.data, sizeof(response));

    if (nbytes != sizeof(response)) return false;
    if (!response.is_ok()) return false;

    return response.parse(aligned);
  }

  bool cancel_goto() {
    using cancel_goto_t = set_command_t<'M'>;

    const cancel_goto_t command;
    cancel_goto_t::response_t response;

    int nbytes = send_command(command.data, sizeof(command), response.data, sizeof(response));

    if (nbytes != sizeof(response)) return false;
    if (!response.is_ok()) return false;

    return true;
  }

  bool echo(char ch) {
    using echo_t = set_command_t<'K', char, char>;

    const echo_t command{ch};
    echo_t::response_t response;

    int nbytes = send_command(command.data, sizeof(command), response.data, sizeof(response));

printf("%ld %ld %d\n", sizeof(command), sizeof(response), nbytes);

    if (nbytes != sizeof(response)) return false;
    if (!response.is_ok()) return false;
    if (response.data[0] != ch) return false;

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

  tracking_mode_kind tracking_mode = tracking_mode_kind::off;
  float slew_rate[2] = {0, 0};

  enum class state_kind : int {
    no_op,
    slewing,
    moving
  };

  state_kind state = state_kind::no_op;

  alpaca::utcdate_t last_ts = alpaca::utcdate_t::now();

  alpaca::utcdate_t utcdate = alpaca::utcdate_t::now();
  alpaca::utcdate_t utcdate_updated = utcdate;

  simulator_protocol() {
  }

  auto step(float target, float* actual, float delta_time) -> void {
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
        step(target_rightascension, &rightascension, delta_time);
        step(target_declination, &declination, delta_time);

        printf("step(%f) => (%f %f) => (%f %f)\n",
          delta_time, rightascension, declination, target_rightascension, target_declination);

        bool is_slewing = target_rightascension != rightascension || target_declination != declination;
        if (!is_slewing)
          state = state_kind::no_op;

        break;
      }

      case state_kind::moving:
        rightascension += slew_rate[0] * delta_time;  // primary axis
        declination += slew_rate[1] * delta_time;  // secondary axis
        break;
    }
  }

  virtual int send_command(
    const void* in_ptr, int in_size, void* out_ptr, int out_size) {

    const char* in = reinterpret_cast<const char*>(in_ptr);
    char* out = reinterpret_cast<char*>(out_ptr);

    step();

    switch (in[0]) {
      case 'K':
        out[0] = in[1];
        out[1] = '#';
        return 2;

      case 'V':
        out[0] = 1;
        out[1] = 2;
        out[2] = '#';
        return 3;

      case 'm':
        out[0] = 20;
        out[1] = '#';
        return 2;

      case 'h':
        std::construct_at(reinterpret_cast<utcdate_t*>(out), utcdate, alpaca::utcdate_t::now() - utcdate_updated);
        out[8] = '#';
        return 9;

      case 'H':
        if (reinterpret_cast<const set_utcdate_t*>(in)->parse(&utcdate)) {
          out[0] = '#';
          return 1;
          utcdate_updated = alpaca::utcdate_t::now();
        } else {
          return 0;
        }

      case 'w':
        std::construct_at(reinterpret_cast<location_t*>(out), latitude, longitude);
        out[8] = '#';
        return 9;

      case 'W':
        if (reinterpret_cast<const set_location_t*>(in)->parse(&latitude, &longitude)) {
          out[0] = '#';
          return 1;
        } else {
          return 0;
        }

      case 'E':
        std::snprintf(out, out_size + 1, "%04X,%04X#",
          degree_to_nexstar(rightascension, false),
          degree_to_nexstar(declination, false));
        return 10;

      case 'e':
        std::snprintf(out, out_size + 1, "%08X,%08X#",
          degree_to_nexstar(rightascension, true),
          degree_to_nexstar(declination, true));
        return 18;

      case 'Z':
      {
        float azimuth, altitude;

        alpaca::astronomy::ra_de_to_azm_alt(
          alpaca::utcdate_t::now(),
          rightascension, declination,
          latitude, longitude,
          &azimuth, &altitude);

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
        out[0] = static_cast<char>(tracking_mode);
        out[1] = '#';
        return 2;

      case 'T':
        tracking_mode = static_cast<tracking_mode_kind>(in[1]);
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
        switch (static_cast<passthrough_command_kind>(in[3])) {
          case passthrough_command_kind::slew_variable_positive:
          case passthrough_command_kind::slew_variable_negative:
          {
            const slew_variable_command_t* slew_command = reinterpret_cast<const slew_variable_command_t*>(in);

            float rate;
            int axis;

            if (slew_command->parse(&axis, &rate)) {
              slew_rate[axis] = rate;

              state = rate != 0 ? state_kind::moving : state_kind::no_op;
            } else {
              state = state_kind::no_op;
            }

            out[0] = '#';
            return 1;
          }

          default:
            break;
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
    const void* in, int in_size, void* out, int out_size) {

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

  virtual ~celestron_telescope()
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

  // telescope

  // read-only properties
  virtual float get_altitude() const {
    float azm, alt;

    check_op(protocol->get_azm_alt(&azm, &alt, false));

    return alt;
  }

  virtual float get_azimuth() const {
    float azm, alt;

    check_op(protocol->get_azm_alt(&azm, &alt, false));

    return azm;
  }

  virtual float get_declination() const {
    float ra, de;

    check_op(protocol->get_ra_de(&ra, &de, false));

    return de;
  }

  virtual float get_rightascension() const {
    float ra, de;

    check_op(protocol->get_ra_de(&ra, &de, false));

    return ra;
  }

  virtual bool get_athome() const {
    return false;
  }

  virtual bool get_atpark() const {
    return false;
  }

  virtual bool get_ispulseguiding() const {
    return false;
  }
  virtual bool get_slewing() const {
    bool is_slewing;
    check_op(protocol->is_goto_in_progress(&is_slewing));

    return is_slewing;
  }

  virtual float get_siderealtime() const {
    float latitude, longitude;

    check_op(protocol->get_location(&latitude, &longitude));

    return alpaca::astronomy::to_lst(alpaca::utcdate_t::now(), longitude) / 15.0f;
  }

  virtual alpaca::destination_side_of_pier_t get_destinationsideofpier(
    float rightascension, float declination) const {
    return alpaca::destination_side_of_pier_t::pier_unknown;
  }

  // read-wrie properties
  virtual float get_sitelatitude() const {
    float latitude, longitude;

    check_op(protocol->get_location(&latitude, &longitude));

    return latitude;
  }

  virtual void put_sitelatitude(float angle) {
    float latitude, longitude;

    check_op(protocol->get_location(&latitude, &longitude));
    check_op(protocol->set_location(angle, longitude));
  }

  virtual float get_sitelongitude() const {
    float latitude, longitude;

    check_op(protocol->get_location(&latitude, &longitude));

    return longitude;
  }

  virtual void put_sitelongitude(float angle) {
    float latitude, longitude;

    check_op(protocol->get_location(&latitude, &longitude));
    check_op(protocol->set_location(latitude, angle));
  }

  float targetdeclination = 100;
  float targetrightascension = 100;
  virtual float get_targetdeclination() const {
    check_set(targetdeclination < 100);
    return targetdeclination;
  }

  virtual void put_targetdeclination(float targetdeclination) {
    this->targetdeclination = targetdeclination;
  }

  virtual float get_targetrightascension() const {
    check_set(targetrightascension < 100);
    return targetrightascension;
  }

  virtual void put_targetrightascension(float targetrightascension) {
    this->targetrightascension = targetrightascension;
  }

  virtual bool get_tracking() const {
    tracking_mode_kind mode;
    check_op(protocol->get_tracking_mode(&mode));

    return mode != tracking_mode_kind::off;
  }

  virtual void put_tracking(bool tracking) {
    tracking_mode_kind mode = tracking ? tracking_mode_kind::eq_north : tracking_mode_kind::off;

    check_op(protocol->set_tracking_mode(mode));
  }

  virtual alpaca::driver_rate_t get_trackingrate() const {
    return alpaca::driver_rate_t::sidereal;
  }

  virtual void put_trackingrate(alpaca::driver_rate_t) {
  }

  virtual void get_utctm(alpaca::utcdate_t* utcdate) const {
    check_op(protocol->get_utcdate(utcdate));
  }

  virtual void put_utctm(alpaca::utcdate_t utcdate) {
    check_op(protocol->set_utcdate(utcdate));
  }

  // operations
  virtual void abortslew() {
    check_op(protocol->cancel_goto());
  }

  virtual void moveaxis(int axis, float rate) {
    check_op(protocol->slew_variable(axis, rate));
  }

  virtual void slewtoaltazasync(float altitude, float azimuth) {
  }

  virtual void slewtocoordinatesasync(float rightascension, float declination) {
    targetrightascension = rightascension;
    targetdeclination = declination;
    check_op(protocol->goto_ra_de(rightascension, declination, false));
  }

  virtual void slewtotargetasync() {
    check_op(protocol->goto_ra_de(targetrightascension, targetdeclination, false));
  }

  virtual void synctoaltaz(float altitude, float azimuth) {
  }

  virtual void synctocoordinates(
    float rightascension, float declination) {
    targetrightascension = rightascension;
    targetdeclination = declination;

    check_op(protocol->goto_ra_de(rightascension, declination, false));
  }

  virtual void synctotarget() {
    //protocol->goto_ra_de(target_ra, target_de);
  }
};

}  // namespace celestron

#endif  // INCLUDE_CELESTRON_CELESTRON_HPP_
