import os
import struct
import sys
import termios
import time

PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00"
DURATION = float(sys.argv[2]) if len(sys.argv) > 2 else 8.0

def main() -> int:
    fd = os.open(PORT, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        attrs = termios.tcgetattr(fd)
        attrs[0] = 0
        attrs[1] = 0
        attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
        attrs[3] = 0
        attrs[4] = termios.B921600
        attrs[5] = termios.B921600
        attrs[6][termios.VMIN] = 0
        attrs[6][termios.VTIME] = 0
        termios.tcsetattr(fd, termios.TCSANOW, attrs)

        buf = bytearray()
        end = time.time() + DURATION
        while time.time() < end:
            try:
                chunk = os.read(fd, 256)
                if chunk:
                    buf.extend(chunk)
            except BlockingIOError:
                pass

            while len(buf) >= 48:
                if buf[0] != 0xD1 or buf[1] != 0x5B:
                    del buf[0]
                    continue

                frame = bytes(buf[:48])
                checksum = 0
                for b in frame[2:45]:
                    checksum ^= b

                if frame[46] != 0x6B or frame[47] != 0x1D or checksum != frame[45]:
                    del buf[0]
                    continue

                flags = frame[2]
                seq = frame[3]
                raw_x, raw_y, err_x, err_y, yaw_add, parsed, rx = struct.unpack_from("<HHhhhHH", frame, 4)
                sw0, sw1 = frame[20], frame[21]
                ch0, ch1, ch2, ch3 = struct.unpack_from("<hhhh", frame, 22)
                behaviour = frame[30]
                manual_yaw_add, manual_pitch_add = struct.unpack_from("<hh", frame, 31)
                yaw_mode = frame[35]
                pitch_mode = frame[36]
                yaw_set, pitch_set, yaw_current, pitch_current = struct.unpack_from("<hhhh", frame, 37)
                print(
                    "diag flags=%d seq=%d sw=(%d,%d) ch=(%d,%d,%d,%d) behaviour=%d raw=(%d,%d) err=(%d,%d) vision_add=(%d,%d) manual_add=(%d,%d) mode=(%d,%d) set=(%d,%d) current=(%d,%d) parsed=%d rx=%d"
                    % (flags, seq, sw0, sw1, ch0, ch1, ch2, ch3, behaviour, raw_x, raw_y, err_x, err_y, yaw_add, 0, manual_yaw_add, manual_pitch_add, yaw_mode, pitch_mode, yaw_set, pitch_set, yaw_current, pitch_current, parsed, rx),
                    flush=True,
                )
                del buf[:48]

            time.sleep(0.02)
    finally:
        os.close(fd)

    return 0

if __name__ == "__main__":
    raise SystemExit(main())
