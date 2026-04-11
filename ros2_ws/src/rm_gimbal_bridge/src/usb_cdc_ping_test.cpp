#include <array>
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
#include <vector>

namespace {

constexpr uint8_t kReqHead0 = 0xA5U;
constexpr uint8_t kReqHead1 = 0x5AU;
constexpr uint8_t kReqCmd = 0x01U;
constexpr uint16_t kReqPayload = 0x1234U;
constexpr uint8_t kReqTail = 0x96U;

constexpr uint8_t kAckHead0 = 0x5AU;
constexpr uint8_t kAckHead1 = 0xA5U;
constexpr uint8_t kAckCmd = 0x81U;
constexpr uint8_t kAckTail = 0x69U;

constexpr uint8_t kAckStatusOk = 0U;
constexpr uint8_t kAckStatusCrcError = 1U;

constexpr int kSendHz = 100;
constexpr int kTimeoutMs = 50;

#pragma pack(push, 1)
struct UsbTestRequestFrame {
  uint8_t head0;
  uint8_t head1;
  uint8_t cmd;
  uint8_t seq;
  uint16_t payload;
  uint8_t crc8;
  uint8_t tail;
};

struct UsbTestAckFrame {
  uint8_t head0;
  uint8_t head1;
  uint8_t cmd;
  uint8_t seq;
  uint8_t status;
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
  fd = open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
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
  tty.c_iflag &= ~(INLCR | ICRNL | IGNCR);
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

struct PendingSlot {
  bool active = false;
  std::chrono::steady_clock::time_point sent_at {};
};

}  // namespace

int main(int argc, char **argv) {
  std::string device = "/dev/ttyACM0";
  if (argc > 1) {
    device = argv[1];
  }

  int serial_fd = -1;
  if (!OpenSerial(device, serial_fd)) {
    return 1;
  }

  std::array<PendingSlot, 256> pending {};
  std::vector<uint8_t> rx_buffer;
  rx_buffer.reserve(512);

  uint8_t next_seq = 0U;
  uint64_t sent_count = 0U;
  uint64_t ack_count = 0U;
  uint64_t crc_error_count = 0U;
  uint64_t timeout_count = 0U;

  auto last_send = std::chrono::steady_clock::now();
  auto last_log = last_send;

  while (true) {
    const auto now = std::chrono::steady_clock::now();

    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_send).count() >=
        (1000 / kSendHz)) {
      UsbTestRequestFrame request {};
      request.head0 = kReqHead0;
      request.head1 = kReqHead1;
      request.cmd = kReqCmd;
      request.seq = next_seq;
      request.payload = kReqPayload;
      request.crc8 = Crc8(&request.cmd, 4U);
      request.tail = kReqTail;

      const ssize_t written = write(serial_fd, &request, sizeof(request));
      if (written == static_cast<ssize_t>(sizeof(request))) {
        pending[next_seq].active = true;
        pending[next_seq].sent_at = now;
        ++sent_count;
        ++next_seq;
      }

      last_send = now;
    }

    uint8_t temp[64];
    const ssize_t nread = read(serial_fd, temp, sizeof(temp));
    if (nread > 0) {
      rx_buffer.insert(rx_buffer.end(), temp, temp + nread);
    }

    while (rx_buffer.size() >= sizeof(UsbTestAckFrame)) {
      if (!(rx_buffer[0] == kAckHead0 && rx_buffer[1] == kAckHead1)) {
        rx_buffer.erase(rx_buffer.begin());
        continue;
      }

      UsbTestAckFrame ack {};
      std::memcpy(&ack, rx_buffer.data(), sizeof(ack));

      if (ack.tail != kAckTail || ack.cmd != kAckCmd) {
        rx_buffer.erase(rx_buffer.begin());
        continue;
      }

      const uint8_t expected_crc = Crc8(&ack.cmd, 3U);
      if (ack.crc8 != expected_crc) {
        rx_buffer.erase(rx_buffer.begin(), rx_buffer.begin() + sizeof(ack));
        continue;
      }

      if (ack.status == kAckStatusOk) {
        ++ack_count;
      } else if (ack.status == kAckStatusCrcError) {
        ++crc_error_count;
      }

      pending[ack.seq].active = false;
      rx_buffer.erase(rx_buffer.begin(), rx_buffer.begin() + sizeof(ack));
    }

    for (auto &slot : pending) {
      if (!slot.active) {
        continue;
      }

      const auto age_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - slot.sent_at).count();
      if (age_ms > kTimeoutMs) {
        slot.active = false;
        ++timeout_count;
      }
    }

    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_log).count() >= 1) {
      std::cout << "[usb_cdc_ping_test] device=" << device
                << " sent_count=" << sent_count
                << " ack_count=" << ack_count
                << " crc_error_count=" << crc_error_count
                << " timeout_count=" << timeout_count << std::endl;
      last_log = now;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  close(serial_fd);
  return 0;
}
