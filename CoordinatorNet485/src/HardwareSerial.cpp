//
//  HardwareSerial.cpp
//  CoordinatorNet485
//
//  Created by Kevin Peck on 2020-07-15.
//  Copyright © 2020 Kevin Peck. All rights reserved.
//
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "HardwareSerial.hpp"

#include <time.h>
#include <chrono>
unsigned long millis() {
    std::chrono::milliseconds uptime(0u);
    struct timespec ts;
    if (clock_gettime(_CLOCK_UPTIME_RAW, &ts) == 0)
    {
      uptime = std::chrono::milliseconds(
        static_cast<unsigned long long>(ts.tv_sec)*1000ULL +
        static_cast<unsigned long long>(ts.tv_nsec)/1000000ULL
       );
    }
    return uptime.count();
}

// Opens the specified serial port, sets it up for binary communication,
// configures its read timeouts, and sets its baud rate.
// Returns a non-negative file descriptor on success, or -1 on failure.
int HardwareSerial::open_serial_port(const char * device, uint32_t baud_rate)
{
  int fd = open(device, O_RDWR | O_NOCTTY);
  if (fd == -1)
  {
    perror(device);
    return -1;
  }
  
  // Flush away any bytes previously read or written.
  int result = tcflush(fd, TCIOFLUSH);
  if (result)
  {
    perror("tcflush failed");   // just a warning, not a fatal error
  }
  
  // Get the current configuration of the serial port.
  struct termios options;
  result = tcgetattr(fd, &options);
  if (result)
  {
    perror("tcgetattr failed");
    close(fd);
    return -1;
  }
  
  // Turn off any options that might interfere with our ability to send and
  // receive raw binary bytes.
  options.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
  options.c_oflag &= ~(ONLCR | OCRNL);
  options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  
  // Set up timeouts: Calls to read() will return as soon as there is
  // at least one byte available or when 100 ms has passed.
  options.c_cc[VTIME] = 1;
  options.c_cc[VMIN] = 0;
  
  // This code only supports certain standard baud rates. Supporting
  // non-standard baud rates should be possible but takes more work.
  switch (baud_rate)
  {
  case 4800:   cfsetospeed(&options, B4800);   break;
  case 9600:   cfsetospeed(&options, B9600);   break;
  case 19200:   cfsetospeed(&options, B19200);   break;
  case 38400:   cfsetospeed(&options, B38400);   break;
  case 115200: cfsetospeed(&options, B115200); break;
  default:
    fprintf(stderr, "warning: baud rate %u is not supported, using 9600.\n",
      baud_rate);
    cfsetospeed(&options, B9600);
    break;
  }
  cfsetispeed(&options, cfgetospeed(&options));
  
  result = tcsetattr(fd, TCSANOW, &options);
  if (result)
  {
    perror("tcsetattr failed");
    close(fd);
    return -1;
  }
  
  return fd;
}


HardwareSerial::HardwareSerial(const char * device, uint32_t baud_rate) {
    fd = HardwareSerial::open_serial_port(device,baud_rate);
}
HardwareSerial::~HardwareSerial() {
    close(fd);
}

int HardwareSerial::available() {
    int bytes;
    ioctl(fd, FIONREAD, &bytes);
    return bytes;
}

void HardwareSerial::setTimeout(unsigned int mstimeout) {
  struct termios options;
  int result = tcgetattr(fd, &options);
  if (result)
  {
    perror("tcgetattr failed");
    close(fd);
  }
  options.c_cc[VTIME] = (int)(mstimeout / 100);

  result = tcsetattr(fd, TCSANOW, &options);
  if (result)
  {
    perror("tcsetattr failed");
    close(fd);
  }
}


// Writes bytes to the serial port, returning 0 on success and -1 on failure.
size_t HardwareSerial::write(const uint8_t *buffer, size_t size) {
  ssize_t result = ::write(fd, buffer, size);
  if (result != (ssize_t)size)
  {
    perror("failed to write to port");
    return -1;
  }
  return 0;
}

// Reads bytes from the serial port.
// Returns after all the desired bytes have been read, or if there is a
// timeout or other error.
// Returns the number of bytes successfully read into the buffer, or -1 if
// there was an error reading.
size_t HardwareSerial::readBytes(uint8_t* buffer, size_t size) {
  size_t received = 0;
  while (received < size)
  {
    ssize_t r = read(fd, buffer + received, size - received);
    if (r < 0)
    {
      perror("failed to read from port");
      return -1;
    }
    if (r == 0)
    {
      // Timeout
      break;
    }
    received += r;
  }
  return received;
}

