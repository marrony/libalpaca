// Copyright (C) 2023 Marrony Neris

#include <getopt.h>
#include <iostream>

#include <manager.hpp>
#include <celestron/celestron.hpp>

void print_help(char* cmdline) {
  std::cout << "Usage: " << cmdline << " [options]" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "Alpaca Server for Raspberry PI" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -d, --device <string>  USB device (default: \"/dev/ttyUSB0\")" << std::endl;
  std::cout << "  -b, --baud <number>    Baud rate (default: 9600)" << std::endl;
  std::cout << "  -p, --port <number>    Port to listen (default: 11111)" << std::endl;
  std::cout << "  -c, --conform          Runs in conform mode (default: false)" << std::endl;
  std::cout << "  -h, --help             Display help" << std::endl;
}

int main(int argc, char** argv) {
  const char* short_options = "h::p::d::b::c::";
  const struct option long_options[] = {
    {"help",    no_argument,       NULL, 'h'},
    {"port",    required_argument, NULL, 'p'},
    {"device",  required_argument, NULL, 'd'},
    {"baud",    required_argument, NULL, 'b'},
    {"conform", no_argument,       NULL, 'c'},
  };

  std::string device;
  int baud = 9600;
  int port = 11111;
  bool conform = false;

  int next_option;
  do {
    next_option = getopt_long(argc, argv, short_options, long_options, NULL);

    switch (next_option) {
      case 'p':
        port = alpaca::util::parse_int(optarg, port);
        break;

      case 'd':
        device = optarg;
        break;

      case 'b':
        baud = alpaca::util::parse_int(optarg, baud);
        break;

      case 'c':
        conform = true;
        break;

      case '?':
      case 'h':
        print_help(argv[0]);
        return 0;
    }
  } while (next_option != -1);

  std::cout << "Listening on port " << port << std::endl;

  if (conform)
    std::cout << "Running in conform mode" << std::endl;

  std::shared_ptr<celestron::nexstar_protocol> protocol;

  if (conform)
    protocol = std::make_shared<celestron::simulator_protocol>();
  else
    protocol = std::make_shared<celestron::serial_protocol>(device, baud);

  alpaca::telescopeinfo_t info = {
    .description = "Generic Celestron",
    .driverinfo = "Generic Celestron",
    .driverversion = "0.0.1",
    .interfaceversion = 2,
    .name = "Generic Celestron",
    .alignmentmode = alpaca::alignment_mode_t::german,
    .aperturearea = 3.14159f * 0.075f * 0.075f,
    .aperturediameter = 0.15f,
    .focallength = 1500,
    .equatorialsystem = alpaca::equatorial_system_t::jnow,
    .axisrates = {{
      .minimum = 0,
      .maximum = 8,
    }},
    .trackingrates = {},
    .flags = alpaca::telescope_flags_t::CAN_SLEW_ASYNC | alpaca::telescope_flags_t::CAN_SLEW_ALTAZ_ASYNC |
             alpaca::telescope_flags_t::CAN_SYNC | alpaca::telescope_flags_t::CAN_SYNC_ALTAZ |
             alpaca::telescope_flags_t::CAN_SET_TRACKING | alpaca::telescope_flags_t::CAN_MOVE_AXIS_0 |
             alpaca::telescope_flags_t::CAN_MOVE_AXIS_1
  };

  celestron::celestron_telescope tel0(info, protocol);

  alpaca::device_manager manager;
  manager.add_telescope(&tel0);
  return manager.run(port);
}
