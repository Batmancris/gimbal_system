#include "target_state.h"

static target_state_t target_state = {0};

void TargetState_Init(void)
{
    target_state.valid = 0U;
    target_state.fresh = 0U;
    target_state.seq = 0U;
    target_state.raw_x = 0U;
    target_state.raw_y = 0U;
    target_state.filtered_x = 0.0f;
    target_state.filtered_y = 0.0f;
    target_state.last_update_tick = 0U;
}

void TargetState_Update(void)
{
    vision_input_frame_t frame;

    // 一次性取完当前收到的帧，只保留最新目标；这里更看重低延迟，
    // 不回放旧目标位置。
    while (VisionInput_FetchFrame(&frame))
    {
        target_state.raw_x = frame.x;
        target_state.raw_y = frame.y;
        target_state.seq = frame.seq;
        target_state.last_update_tick = frame.tick;
        target_state.fresh = 1U;
        target_state.valid = 1U;

        if (target_state.filtered_x == 0.0f && target_state.filtered_y == 0.0f)
        {
            target_state.filtered_x = (fp32)frame.x;
            target_state.filtered_y = (fp32)frame.y;
        }
        else
        {
            target_state.filtered_x +=
              ((fp32)frame.x - target_state.filtered_x) * TARGET_STATE_SMOOTH_ALPHA;
            target_state.filtered_y +=
              ((fp32)frame.y - target_state.filtered_y) * TARGET_STATE_SMOOTH_ALPHA;
        }
    }

    if (target_state.valid &&
        (HAL_GetTick() - target_state.last_update_tick) > TARGET_STATE_TIMEOUT_MS)
    {
        // 超时后目标置为无效，让 gimbal_task 重置视觉控制，
        // 避免继续跟随过期坐标。
        target_state.valid = 0U;
        target_state.fresh = 0U;
    }
}

void TargetState_Inject(uint16_t x, uint16_t y)
{
    target_state.raw_x = x;
    target_state.raw_y = y;
    target_state.filtered_x = (fp32)x;
    target_state.filtered_y = (fp32)y;
    target_state.valid = 1U;
    target_state.fresh = 1U;
    target_state.last_update_tick = HAL_GetTick();
}

void TargetState_SetReady(uint8_t ready)
{
    target_state.fresh = ready ? 1U : 0U;

    if (ready == 0U)
    {
        target_state.valid = 0U;
    }
}

const target_state_t *TargetState_Get(void)
{
    return &target_state;
}

uint8_t TargetState_FetchPosition(uint16_t *x, uint16_t *y)
{
    if (x == NULL || y == NULL || target_state.valid == 0U)
    {
        return 0U;
    }

    *x = (uint16_t)target_state.filtered_x;
    *y = (uint16_t)target_state.filtered_y;
    target_state.fresh = 0U;
    return 1U;
}
