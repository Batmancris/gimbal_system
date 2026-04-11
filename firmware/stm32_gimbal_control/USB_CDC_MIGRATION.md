# USB CDC Migration Guide

## Purpose

This document records the safe migration path for moving the current lower-level
communication path from UART to USB CDC in `firmware/stm32_gimbal_control`.

Current status:

- Verified mainline communication: `UART`
- USB CDC status: experimental bring-up / minimum ping-ack test path
- Reference implementation source: `../../archive/historical_code/tianboard_s/tianboard_s/USB_DEVICE`

The goal is to migrate safely without breaking the currently working gimbal
firmware control chain.

## Current Mainline

The current validated lower-level control path is:

- DBUS remote control
- CAN motor control
- IMU / attitude pipeline
- UART-based vision input

This means:

- `USART1` is still part of the current validated communication path
- USB CDC should be added first
- UART should only be removed after USB CDC is fully validated

## Reference Source in `archive/historical_code/tianboard_s`

The following files already exist in the reference project and can be used as
the migration reference:

- `archive/historical_code/tianboard_s/tianboard_s/USB_DEVICE/App/usb_device.c`
- `archive/historical_code/tianboard_s/tianboard_s/USB_DEVICE/App/usb_device.h`
- `archive/historical_code/tianboard_s/tianboard_s/USB_DEVICE/App/usbd_cdc_if.c`
- `archive/historical_code/tianboard_s/tianboard_s/USB_DEVICE/App/usbd_cdc_if.h`
- `archive/historical_code/tianboard_s/tianboard_s/USB_DEVICE/App/usbd_desc.c`
- `archive/historical_code/tianboard_s/tianboard_s/USB_DEVICE/App/usbd_desc.h`
- `archive/historical_code/tianboard_s/tianboard_s/USB_DEVICE/Target/usbd_conf.c`
- `archive/historical_code/tianboard_s/tianboard_s/USB_DEVICE/Target/usbd_conf.h`
- `archive/historical_code/tianboard_s/tianboard_s/Middlewares/ST/STM32_USB_Device_Library/...`

## What `firmware/stm32_gimbal_control` Already Has

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

## What `firmware/stm32_gimbal_control` Still Lacks

The following USB Device files are still missing from the main lower-level
project:

- `USB_DEVICE/App/usb_device.c`
- `USB_DEVICE/App/usb_device.h`
- `USB_DEVICE/App/usbd_cdc_if.c`
- `USB_DEVICE/App/usbd_cdc_if.h`
- `USB_DEVICE/App/usbd_desc.c`
- `USB_DEVICE/App/usbd_desc.h` (optional depending on CubeMX layout)
- `USB_DEVICE/Target/usbd_conf.c`
- `USB_DEVICE/Target/usbd_conf.h`
- `Middlewares/ST/STM32_USB_Device_Library/...`

In addition, the STM32 CubeMX / IOC configuration still needs:

- `USB_OTG_FS` enabled
- `USB_DEVICE` middleware enabled
- `CDC` class enabled
- `PA11 = USB_DM`
- `PA12 = USB_DP`

## Safe Migration Steps

### Phase 1: USB device bring-up only

1. Generate USB CDC Device files from CubeMX for `firmware/stm32_gimbal_control`
2. Add generated files into the project Makefile
3. Replace the weak placeholder `MX_USB_DEVICE_Init()` with the generated one
4. In generated `usbd_cdc_if.c`, route received bytes into:

```c
VisionInput_FeedBytes(Buf, (uint16_t)(*Len));
```

5. Implement `UsbCdcTest_SendBytes(...)` by calling the USB CDC transmit path

Success standard:

- board enumerates on host
- ping/ack works over USB CDC

### Phase 2: Dual-path coexistence

Keep both paths alive:

- UART remains available
- USB CDC becomes test path

At this phase:

- do not remove `USART1`
- do not remove `USART1_IRQHandler`
- do not remove `MX_USART1_UART_Init()`

Success standard:

- USB CDC ping/ack stable
- UART mainline still works

### Phase 3: Vision data over USB CDC

After minimum USB transport is stable:

- route vision frames through USB CDC
- keep the same parser entry in `VisionInput_FeedBytes()`
- validate:
  - frame parsing
  - target state update
  - timeout behavior

Success standard:

- upper and lower computer exchange vision frames reliably over USB CDC

### Phase 4: UART deprecation

Only after USB CDC is fully validated:

- update README / docs to mark USB CDC as new mainline
- then decide whether UART becomes:
  - fallback path
  - or removable legacy path

Do not remove UART before this point.

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

Use `archive/historical_code/tianboard_s` as the USB CDC reference source, but keep `firmware/stm32_gimbal_control`
as the only main lower-level firmware project.

That means:

- do not replace `firmware/stm32_gimbal_control` with `archive/historical_code/tianboard_s`
- do not delete UART yet
- migrate transport safely in stages
