#include "coordinate.h"

#include "target_state.h"

TargetPosition target_position = {0};

static void refresh_legacy_view(void)
{
    const target_state_t *state = TargetState_Get();

    target_position.data_ready = state->valid;
    target_position.object_x = (uint16_t)state->filtered_x;
    target_position.object_y = (uint16_t)state->filtered_y;
}

const TargetPosition *get_target_position(void)
{
    refresh_legacy_view();
    return &target_position;
}

uint8_t fetch_target_position(uint16_t *x, uint16_t *y)
{
    uint8_t ok = TargetState_FetchPosition(x, y);
    refresh_legacy_view();
    return ok;
}

void set_target_position(uint16_t x, uint16_t y)
{
    TargetState_Inject(x, y);
    refresh_legacy_view();
}

void set_target_data_ready(uint8_t ready)
{
    TargetState_SetReady(ready);
    target_position.data_ready = ready;
}
