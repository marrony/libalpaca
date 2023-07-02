// Copyright (C) 2023 Marrony Neris

#ifndef INCLUDE_ASTRONOMY_HPP_
#define INCLUDE_ASTRONOMY_HPP_

#include <iostream>
#include <cstdint>
#include <cmath>

#include <time.hpp>

namespace alpaca {
namespace astronomy {

struct dms_t {
  int degree;
  int minute;
  int second;

  dms_t(int degree = 0, int minute = 0, int second = 0)
  : degree(degree)
  , minute(minute)
  , second(second)
  { }

  dms_t(float angle) {
    angle = std::fmod(angle, 360.0f);

    int angle_secs = angle >= 0
      ? static_cast<int>(angle * 3600 + 0.5f)
      : static_cast<int>(angle * 3600 - 0.5f);

    degree = angle_secs / 3600;

    int secs = std::abs(angle_secs) % 3600;
    minute = secs / 60;
    second = secs % 60;
  }

  float to_decimal() const {
    float fraction = minute/60.0f + second/3600.0f;
    return degree < 0 ? degree - fraction : degree + fraction;
  }
};

void ra_de_to_azm_alt(
  utcdate_t now,
  float ra, float de,
  float lat, float lon,
  float* azm, float* alt) {

  dms_t lstRA = alpaca::to_lst(now, lon);

  float ha = lstRA.to_decimal() - ra;

  if (ha < 0) ha += 360.0f;

//sin(ALT) = sin(DEC)*sin(LAT) + cos(DEC)*cos(LAT)*cos(HA)
//
//ALT = asin(ALT) 
//
//               sin(DEC) - sin(ALT)*sin(LAT)
//cos(A)   =   ---------------------------------
//                   cos(ALT)*cos(LAT)
//
//A = acos(A)
//
//If sin(HA) is negative, then AZ = A, otherwise
//AZ = 360 - A

  float k = M_PI / 180.0f;

  float sin_alt = std::sin(de*k)*std::sin(lat*k) + std::cos(de*k)*std::cos(lat*k)*std::cos(ha*k);
  *alt = std::asin(sin_alt);

  float cos_azm = (std::sin(de*k) - sin_alt*std::sin(lat*k)) / (std::cos(*alt)*std::cos(lat*k));
  *azm = std::acos(cos_azm);

  if (std::sin(ha*k) > 0)
    *azm = 2*M_PI - *azm;
}

void azm_alt_to_ra_de(
  utcdate_t now,
  float azm, float alt,
  float lat, float lon,
  float* ra, float* de) {

  float k = M_PI / 180.0f;
  float k2 = 180.0f / M_PI;

  dms_t lstRA = alpaca::to_lst(now, lon);

  float sin_de = std::sin(alt*k)*std::sin(lat*k) + std::cos(alt*k)*std::cos(lat*k)*std::cos(azm*k);
  *de = std::asin(sin_de);

  float cos_ha = (std::sin(alt*k) - sin_de*std::sin(lat*k)) / (std::cos(*de)*std::cos(lat*k));
  float ha = std::acos(cos_ha)*k2;

  //if (std::sin(azm*k) < 0)
  //  ha += M_PI;

  *ra = lstRA.to_decimal() - ha;
}

}  // namespace astronomy
}  // namespace alpaca

static inline std::ostream& operator<<(std::ostream& os, const alpaca::astronomy::dms_t& dms) {
  os << dms.degree << " " << dms.minute << "' " << dms.second << "\"";
  return os;
}

#endif  // INCLUDE_ASTRONOMY_HPP_
