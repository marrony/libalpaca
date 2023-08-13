// Copyright (C) 2023 Marrony Neris

#ifndef INCLUDE_TIME_HPP_
#define INCLUDE_TIME_HPP_

#include <ctime>

namespace alpaca {

struct utcdate_t {
  std::uint64_t micros;

  utcdate_t& operator+=(int offset_micros) {
    micros += offset_micros;
    return *this;
  }

  constexpr std::int64_t operator-(utcdate_t other) const {
    return micros - other.micros;
  }

  constexpr std::time_t to_time_t(int offset = 0) const {
    return static_cast<std::time_t>((micros+offset) / 1000000);
  }

  static utcdate_t get_system_micros() {
    timeval tv;
    gettimeofday(&tv, NULL);
    return { tv.tv_sec*1000000ull + tv.tv_usec };
  }

  static utcdate_t now() noexcept {
    return get_system_micros();
  }

  static utcdate_t from_time_t(std::time_t time) {
    return { static_cast<std::uint64_t>(time) * 1000000 };
  }

  static utcdate_t from_local_tm(const std::tm* local_tm) {
    std::time_t time = mktime(const_cast<std::tm*>(local_tm));
    return from_time_t(time);
  }

  static utcdate_t from_utc_tm(const std::tm* utc_tm) {
    std::time_t time = timegm(const_cast<std::tm*>(utc_tm));
    return from_time_t(time);
  }

  void to_local_tm(std::tm* local_tm) const {
    std::time_t time = to_time_t();
    localtime_r(&time, local_tm);
  }

  void to_utc_tm(std::tm* utc_tm) const {
    std::time_t time = to_time_t();
    gmtime_r(&time, utc_tm);
  }

  static return_t<utcdate_t> parse_utc(std::string_view utc) {
    std::tm utc_tm;
    float seconds;

    int count = std::sscanf(
      utc.data(),
      "%04d-%02d-%02dT%02d:%02d:%fZ",
      &utc_tm.tm_year, &utc_tm.tm_mon,
      &utc_tm.tm_mday, &utc_tm.tm_hour,
      &utc_tm.tm_min, &seconds);

    if (count != 6) {
      return invalid_value();
    }

    utc_tm.tm_year -= 1900;
    utc_tm.tm_mon -= 1;
    utc_tm.tm_sec = static_cast<int>(seconds);
    utc_tm.tm_isdst = -1;

    return utcdate_t::from_utc_tm(&utc_tm);
  }

  std::string format_utc() const {
    char utc[128];
    std::tm utc_tm;

    to_utc_tm(&utc_tm);

    std::snprintf(
      utc,
      128,
      "%04d-%02d-%02dT%02d:%02d:%02dZ",
      utc_tm.tm_year + 1900, utc_tm.tm_mon + 1, utc_tm.tm_mday,
      utc_tm.tm_hour, utc_tm.tm_min, utc_tm.tm_sec);

    return utc;
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

}  // namespace alpaca

#endif  // INCLUDE_TIME_HPP_
