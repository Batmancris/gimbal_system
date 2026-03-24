#ifndef TARGET_STATE_H
#define TARGET_STATE_H

#include "struct_typedef.h"
#include "vision_input.h"

#define TARGET_STATE_TIMEOUT_MS      100U
#define TARGET_STATE_SMOOTH_ALPHA    0.22f

typedef struct
{
    uint8_t valid;
    uint8_t fresh;
    uint8_t seq;
    uint16_t raw_x;
    uint16_t raw_y;
    fp32 filtered_x;
    fp32 filtered_y;
    uint32_t last_update_tick;
} target_state_t;

void TargetState_Init(void);
void TargetState_Update(void);
void TargetState_Inject(uint16_t x, uint16_t y);
void TargetState_SetReady(uint8_t ready);
const target_state_t *TargetState_Get(void);
uint8_t TargetState_FetchPosition(uint16_t *x, uint16_t *y);

#endif
