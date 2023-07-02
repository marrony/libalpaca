// Copyright (C) 2023 Marrony Neris

#ifndef INCLUDE_TIME_HPP_
#define INCLUDE_TIME_HPP_

namespace alpaca {

struct utcdate_t {
  std::uint64_t micros;

  static utcdate_t get_system_micros() {
    timeval tv;
    gettimeofday(&tv, NULL);
    return { tv.tv_sec*1000000ull + tv.tv_usec };
  }

  constexpr std::int64_t operator-(utcdate_t other) const {
    return micros - other.micros;
  }

  static utcdate_t now() noexcept {
    return get_system_micros();
  }
};

// Julian date clock
struct jdate_t {
  std::uint64_t micros;

  //jdate_t(utcdate_t utc)
  //: micros(utc.micros + jdiff_micros)
  //{ }

  // difference between Julian epoch and Gregorian epoch in hours.
  // Julian epoch    = -4713-11-24T12:00:00 UTC
  // Gregorian epoch = +1970-01-01T00:00:00 UTC
  //
  // 1970-01-01T00:00:00 - (-4713-11-24T12:00:00 UTC)
  constexpr static std::uint64_t hours_to_micros = 3600000000ull;
  constexpr static std::uint64_t days_to_micros = 86400000000ull;
  constexpr static std::uint64_t jdiff_hours = 58574100ull;
  constexpr static std::uint64_t jdiff_micros = jdiff_hours * hours_to_micros;

  // todo: find a way to use float
  constexpr double julian_day() const {
    return static_cast<double>(micros) / days_to_micros;
  }

  constexpr static jdate_t from_utc(utcdate_t utc) {
    return { utc.micros + jdiff_micros };
  }

  static jdate_t now() noexcept {
    return from_utc(utcdate_t::now());
  }
};

constexpr float static to_gmst(jdate_t jdate) {
  double JD = jdate.julian_day();
  float diff = JD - 2451545.0f;
  float T = diff / 36525.0f;
  float theta0 = 280.46061837f + 360.98564736629f * diff + (0.000387933f * T * T) - (T * T * T / 38710000.0f);
  float angle = std::fmod(theta0, 360.0);

  if (angle < 0.0)
    angle += 360.0;

  return angle;
}

constexpr float static to_gmst(utcdate_t utc) {
  return to_gmst(jdate_t::from_utc(utc));
}

constexpr static float to_lst(jdate_t jdate, float longitude) {
  return to_gmst(jdate) + longitude;
}

constexpr static float to_lst(utcdate_t utc, float longitude) {
  return to_lst(jdate_t::from_utc(utc), longitude);
}

}  // namespace alpaca

#endif  // INCLUDE_TIME_HPP_
