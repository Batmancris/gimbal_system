#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <termios.h>
#include <thread>
#include <unistd.h>

namespace {

constexpr uint8_t kHead0 = 0xAAU;
constexpr uint8_t kHead1 = 0x55U;
constexpr uint8_t kCmdPitch = 0x01U;
constexpr uint8_t kTail = 0x96U;
constexpr int16_t kPitchPosDeg = 45;
constexpr int16_t kPitchNegDeg = -45;
constexpr int kTogglePeriodSec = 5;

#pragma pack(push, 1)
struct UsbPitchFrame {
  uint8_t head0;
  uint8_t head1;
  uint8_t cmd;
  int16_t pitch_deg;
  uint8_t crc8;
  uint8_t tail;
};
#pragma pack(pop)

uint8_t Crc8(const uint8_t *data, std::size_t len) {
  uint8_t crc = 0x00U;

  for (std::size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) {
      if (crc & 0x80U) {
        crc = static_cast<uint8_t>((crc << 1U) ^ 0x07U);
      } else {
        crc <<= 1U;
      }
    }
  }

  return crc;
}

bool OpenSerial(const std::string &device, int &fd) {
  fd = open(device.c_str(), O_RDWR | O_NOCTTY);
  if (fd < 0) {
    std::cerr << "open(" << device << ") failed: " << std::strerror(errno) << '\n';
    return false;
  }

  termios tty {};
  if (tcgetattr(fd, &tty) != 0) {
    std::cerr << "tcgetattr failed: " << std::strerror(errno) << '\n';
    close(fd);
    fd = -1;
    return false;
  }

  cfsetospeed(&tty, B115200);
  cfsetispeed(&tty, B115200);

  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;
  tty.c_cflag &= ~CRTSCTS;
  tty.c_cflag |= CLOCAL | CREAD;

  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  tty.c_oflag &= ~OPOST;

  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 0;

  if (tcsetattr(fd, TCSANOW, &tty) != 0) {
    std::cerr << "tcsetattr failed: " << std::strerror(errno) << '\n';
    close(fd);
    fd = -1;
    return false;
  }

  tcflush(fd, TCIOFLUSH);
  return true;
}

bool SendPitchDeg(int fd, int16_t pitch_deg) {
  UsbPitchFrame frame {};
  frame.head0 = kHead0;
  frame.head1 = kHead1;
  frame.cmd = kCmdPitch;
  frame.pitch_deg = pitch_deg;
  frame.crc8 = Crc8(&frame.cmd, 3U);
  frame.tail = kTail;

  const ssize_t written = write(fd, &frame, sizeof(frame));
  return written == static_cast<ssize_t>(sizeof(frame));
}

}  // namespace

int main(int argc, char **argv) {
  std::string device = "/dev/ttyACM0";
  if (argc > 1) {
    device = argv[1];
  }

  int fd = -1;
  if (!OpenSerial(device, fd)) {
    return 1;
  }

  int16_t target_deg = kPitchPosDeg;
  while (true) {
    if (!SendPitchDeg(fd, target_deg)) {
      std::cerr << "send failed, errno=" << errno << " (" << std::strerror(errno) << ")" << '\n';
    } else {
      std::cout << "[usb_cdc_pitch_test] device=" << device << " pitch_deg=" << target_deg << '\n';
    }

    target_deg = (target_deg == kPitchPosDeg) ? kPitchNegDeg : kPitchPosDeg;
    std::this_thread::sleep_for(std::chrono::seconds(kTogglePeriodSec));
  }

  close(fd);
  return 0;
}
