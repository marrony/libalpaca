// Copyright (C) 2023 Marrony Neris

#include <iostream>
#include <stdexcept>
#include <string_view>
#include <celestron/celestron.hpp>

int main(int argc, char** argv) {
  celestron::serial_protocol serial(argv[1], 9600);

  bool aligned;
  if (!serial.is_aligned(&aligned))
    throw std::invalid_argument("Cannot read alignment");

  std::cout << "Aligned: " << aligned << std::endl;

  int minor, major;
  if (!serial.get_version(&major, &minor))
    throw std::invalid_argument("Cannot read version");

  std::cout << "Major: " << major << std::endl;
  std::cout << "Minor: " << minor << std::endl;

  float ra, de;
  if (!serial.get_ra_dec(&ra, &de, false))
    throw std::invalid_argument("Cannot read RA/DEC");
  std::cout << "RA: " << ra << std::endl;
  std::cout << "DE: " << de << std::endl;

  if (!serial.get_ra_dec(&ra, &de, true))
    throw std::invalid_argument("Cannot read RA/DEC Precise");
  std::cout << "RA: " << ra << " " << dms_t(ra) << std::endl;
  std::cout << "DE: " << de << " " << dms_t(de) << std::endl;

  float azm, alt;
  if (!serial.get_azm_alt(&azm, &alt, false))
    throw std::invalid_argument("Cannot read AZM/ALT");
  std::cout << "AZM: " << azm << std::endl;
  std::cout << "ALT: " << alt << std::endl;

  if (!serial.get_azm_alt(&azm, &alt, true))
    throw std::invalid_argument("Cannot read AZM/ALT Precise");
  std::cout << "AZM: " << azm << " " << dms_t(azm) << std::endl;
  std::cout << "ALT: " << alt << " " << dms_t(alt) << std::endl;

  return 0;
}

