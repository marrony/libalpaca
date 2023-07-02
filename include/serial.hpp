// Copyright (C) 2023 Marrony Neris

#ifndef INCLUDE_SERIAL_HPP_
#define INCLUDE_SERIAL_HPP_

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <vector>
#include <string>

namespace alpaca {

class serial {
  int fd;

  int set_interface_attribs(int speed) {
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
      return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
      return -1;
    }

    return 0;
  }

  /*
  VMIN = 0, VTIME = 0
  No blocking, return immediately with what is available

  VMIN > 0, VTIME = 0
  This will make read() always wait for bytes (exactly how many is determined by VMIN),
  so read() could block indefinitely.

  VMIN = 0, VTIME > 0
  This is a blocking read of any number of chars with a maximum timeout (given by VTIME).
  read() will block until either any amount of data is available, or the timeout occurs.

  VMIN > 0, VTIME > 0
  Block until either VMIN characters have been received, or VTIME after first character has elapsed.
  Note that the timeout for VTIME does not begin until the first character is received.
  */
  int set_blocking(int mcount) {
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
      return - 1;
    }

    tty.c_cc[VMIN] = mcount;
    tty.c_cc[VTIME] = 5;        /* half second timer */

    if (tcsetattr(fd, TCSANOW, &tty) < 0)
      return -1;

    return 0;
  }

 public:
  serial() : fd(-1) { }

  bool is_open() const {
    return fd != -1;
  }

  bool open(std::string_view path, int baudRate) {
    fd = ::open(path.data(), O_RDWR | O_NOCTTY | O_SYNC);

    if (fd < 0)
      return false;

    if (set_interface_attribs(baudRate) < 0)
      return false;

    return true;
  }

  bool close() {
    ::close(fd);
    fd = -1;
    return true;
  }

  int read(char* out, int out_size) {
    const int size = out_size;

    int nbytes;
    while ((nbytes = ::read(fd, out, out_size)) > 0) {
      out += nbytes;
      out_size -= nbytes;
    }

    return size - out_size;
  }

  int write(const char* in, int in_size) {
    return ::write(fd, in, in_size);
  }
};

}  // namespace alpaca

#endif  // INCLUDE_SERIAL_HPP_
