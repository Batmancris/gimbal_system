# firmware

This directory contains the active product-facing firmware layout.

Current reality:

- the active firmware project is `firmware/stm32_gimbal_control/`

Target direction:

```text
firmware/
└── stm32_gimbal_control/
```

Migration status:

- the STM32 firmware mainline has moved out of the old space-containing path
- top-level wrappers now resolve firmware builds to `firmware/stm32_gimbal_control/`
- keep UART as the stable communication mainline unless system-level validation promotes another path
