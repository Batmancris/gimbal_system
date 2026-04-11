# USB CDC Migration Guide

## Purpose

This document records the USB CDC migration state in
`firmware/stm32_gimbal_control`.

Current status:

- Remote-control mainline: `DBUS` / SBUS-like RC frames on `USART3 + DMA + IDLE`
- Upper-to-lower vision mainline: `USB CDC`
- UART status: compatibility path for the shared vision frame parser and validated framing
- Historical USB CDC reference source: Git history or the remote historical `main` branch

The goal is to keep USB CDC as the active upper-to-lower vision path without
breaking the DBUS remote-control chain or the already validated vision frame
format.

## Current Mainline

The current lower-level control path is:

- DBUS remote control
- CAN motor control
- IMU / attitude pipeline
- USB-CDC-based vision input

This means:

- `Chassis/remote_control.c` owns the DBUS/RC receive path
- `USB_DEVICE/App/usbd_cdc_if.c` forwards received USB CDC bytes into `VisionInput_FeedBytes(...)`
- `Src/vision_input.c` owns the shared `0xFA 0xFB ... 0xFC 0xFD` parser
- UART-compatible pieces should only be removed after explicit hardware validation

## Historical Reference Source

The previous `tianboard_s` local reference snapshot has been removed from this
working tree. Use Git history or the remote historical `main` branch if the
original USB CDC reference needs to be inspected again.

## Protocol Hooks Already Present

The following pieces are already prepared in `firmware/stm32_gimbal_control`:

- `Src/usb_cdc_test.c`
- `Src/usb_cdc_test.h`
- `Src/vision_input.c`
- `Src/vision_input.h`
- `Src/main.c`

These files already provide:

- a minimum USB ping/ack parser
- a transport abstraction `UsbCdcTest_SendBytes(...)`
- a unified byte feed entry:
  - `VisionInput_FeedBytes(const uint8_t *data, uint16_t len)`

This means the protocol-side test framework is already in place.

## USB Device Files Already Present

The main lower-level project now contains the USB Device files and Makefile
entries used by the active CDC path:

- `USB_DEVICE/App/usb_device.c`
- `USB_DEVICE/App/usb_device.h`
- `USB_DEVICE/App/usbd_cdc_if.c`
- `USB_DEVICE/App/usbd_cdc_if.h`
- `USB_DEVICE/App/usbd_desc.c`
- `USB_DEVICE/App/usbd_desc.h`
- `USB_DEVICE/Target/usbd_conf.c`
- `USB_DEVICE/Target/usbd_conf.h`

## Safe Migration Steps

### Phase 1: USB device bring-up only

Status: completed in the current working tree.

Completed standard:

- board enumerates on host
- ping/ack works over USB CDC
- generated USB Device files are part of the firmware project

### Phase 2: Dual-path coexistence

Keep both paths alive:

- DBUS remote control remains available
- USB CDC carries the upper-to-lower vision path
- UART-compatible parser/framing code remains available

At this phase:

- do not remove `remote_control.c`
- do not remove `VisionInput_FeedBytes(...)`
- do not remove the validated `0xFA 0xFB ... 0xFC 0xFD` frame parser

Success standard:

- USB CDC ping/ack stable
- DBUS remote-control path still works
- bridge target frames update `target_state`

### Phase 3: Vision data over USB CDC

- route vision frames through USB CDC
- keep the same parser entry in `VisionInput_FeedBytes()`
- validate:
  - frame parsing
  - target state update
  - timeout behavior

Success standard:

- upper and lower computer exchange vision frames reliably over USB CDC

### Phase 4: UART-compatible path retirement

Only after USB CDC is fully validated:

- keep README / docs marking DBUS remote control and USB-CDC vision input as the current mainline
- then decide whether UART-compatible pieces become:
  - fallback path
  - or removable legacy path

Do not remove UART-compatible code before this point.

## Recommended Code Hook Points

### In `main.c`

Safe final target:

- keep `MX_USART1_UART_Init()` until migration is complete
- call `MX_USB_DEVICE_Init()` after low-level peripheral init
- keep `VisionInput_StartReception()` until UART is retired

### In `usbd_cdc_if.c`

Recommended receive hook:

```c
VisionInput_FeedBytes(Buf, (uint16_t)(*Len));
```

Recommended transmit hook:

- `UsbCdcTest_SendBytes(...)`

### In `vision_input.c`

No transport-specific parsing should live here beyond the common byte feed.

`VisionInput_FeedBytes(...)` should remain the shared byte entry for:

- UART
- USB CDC

## Final Recommendation

Use Git history or the remote historical `main` branch as the USB CDC reference
source when needed, but keep `firmware/stm32_gimbal_control` as the only main
lower-level firmware project.

That means:

- do not reintroduce a local `archive/historical_code/tianboard_s` runtime tree
- do not delete UART-compatible parser/framing paths without an explicit hardware validation record
- migrate transport safely in stages
