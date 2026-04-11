# firmware

This directory is the product-facing transition anchor for the future firmware layout.

Current reality:

- the active firmware project is still `Gimbal control/`

Target direction:

```text
firmware/
└── stm32_gimbal_control/
```

Why it is not moved yet:

- the current Makefile and local workflows already reference `Gimbal control/`
- the current directory name contains a space, which is not ideal, but renaming it prematurely would be high risk
- the move should happen only after validating local and remote references
