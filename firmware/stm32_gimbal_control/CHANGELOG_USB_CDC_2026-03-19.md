# USB CDC Diagnostic Update - 2026-03-19

## Summary

This document records the STM32 USB CDC bring-up and diagnostic progress completed on `2026-03-19`.

## What Was Added

- USB Device / CDC base files were migrated into `firmware/stm32_gimbal_control`
- HAL PCD / LL USB drivers were added to the STM32 HAL tree
- `vision_input` gained a unified byte feed entry for future UART/USB coexistence
- `usb_cdc_test` diagnostic module was added
- `usbd_cdc_if.c` was wired to forward incoming USB data into the diagnostic path

## Diagnostics Implemented

### 1. Ping/ACK diagnostic

- Host sends a fixed ping frame
- STM32 parses the test frame and attempts to send an ACK frame back

### 2. Forced ACK on RX

- On every `CDC_Receive_FS()` callback, STM32 can immediately try to send a fixed ACK
- This was used to separate parser issues from transport issues

### 3. Heartbeat diagnostic

- STM32 sends `HB\r\n` every 500 ms
- Start delay after boot: 1000 ms
- Verified on RDK-X5 as:

```text
48 42 0d 0a
```

### 4. RX echo diagnostic

- STM32 echoes the first 16 bytes of every received packet
- Verified with host payload:

```text
PING1234\r\n
```

Host observed echoed bytes:

```text
50 49 4e 47 31 32 33 34 0d 0a
```

## Verified Results

- USB CDC enumeration: success
- Device node on RDK-X5: `/dev/ttyACM0`
- STM32 -> RDK-X5 TX heartbeat: success
- RDK-X5 -> STM32 RX path: success
- STM32 RX echo back to host: success

## Current Conclusion

The USB CDC **transport layer is now bidirectionally working**.

What remains for later:

- Replace or retire the UART mainline path when ready
- Route `rm_gimbal_bridge` output into the new USB CDC transport
- Reconnect USB CDC transport to the existing visual data path without changing control law behavior
