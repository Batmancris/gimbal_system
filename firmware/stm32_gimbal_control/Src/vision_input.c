#include "vision_input.h"
#include "usb_cdc_test.h"

#include <string.h>

typedef struct
{
    uint8_t dma_rx_buf[VISION_DMA_BUFFER_SIZE];
    uint8_t ring_buf[VISION_RING_BUFFER_SIZE];
    uint16_t ring_head;
    uint16_t ring_tail;
    uint8_t frame_ready;
    vision_input_frame_t latest_frame;
    vision_input_status_t status;
} vision_input_context_t;

static vision_input_context_t vision_input_ctx = {0};

static uint16_t ring_next_index(uint16_t index);
static uint16_t ring_count(void);
static void ring_push_bytes(const uint8_t *data, uint16_t len);
static uint8_t ring_peek(uint16_t offset, uint8_t *value);
static void ring_drop(uint16_t len);
static uint8_t frame_checksum(const uint8_t *frame);
static void parse_ring_frames(void);
static void store_frame(uint16_t x, uint16_t y, uint8_t seq, uint8_t checksum, uint8_t enhanced);

void USART1_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) != RESET)
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);
        VisionInput_HandleIdleInterrupt();
    }
}

void VisionInput_StartReception(void)
{
    memset(&vision_input_ctx, 0, sizeof(vision_input_ctx));

    HAL_UART_DMAStop(&huart1);
    HAL_UART_Receive_DMA(&huart1, vision_input_ctx.dma_rx_buf, VISION_DMA_BUFFER_SIZE);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
}

void VisionInput_HandleIdleInterrupt(void)
{
    uint16_t received_len;

    // IDLE 表示这一段接收暂时结束：先把 DMA 缓冲区喂给解析器，
    // 然后立刻重启 DMA，保证 USB/UART 后续数据还能继续进来。
    HAL_UART_DMAStop(&huart1);
    received_len = (uint16_t)(VISION_DMA_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart1.hdmarx));

    if (received_len > 0U && received_len <= VISION_DMA_BUFFER_SIZE)
    {
        VisionInput_FeedBytes(vision_input_ctx.dma_rx_buf, received_len);
    }

    memset(vision_input_ctx.dma_rx_buf, 0, sizeof(vision_input_ctx.dma_rx_buf));
    HAL_UART_Receive_DMA(&huart1, vision_input_ctx.dma_rx_buf, VISION_DMA_BUFFER_SIZE);
}

void VisionInput_FeedBytes(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0U)
    {
        return;
    }

    vision_input_ctx.status.rx_bytes += len;
    vision_input_ctx.status.last_rx_tick = HAL_GetTick();

    // Stage 2 USB CDC ping/ack test traffic is parsed in the dedicated test module.
    UsbCdcTest_FeedBytes(data, len);

    // Keep the existing vision protocol parser alive for UART compatibility.
    ring_push_bytes(data, len);
    parse_ring_frames();
}

uint8_t VisionInput_FetchFrame(vision_input_frame_t *frame)
{
    if (frame == NULL || vision_input_ctx.frame_ready == 0U)
    {
        return 0U;
    }

    __disable_irq();
    *frame = vision_input_ctx.latest_frame;
    vision_input_ctx.frame_ready = 0U;
    __enable_irq();

    return 1U;
}

const vision_input_status_t *VisionInput_GetStatus(void)
{
    return &vision_input_ctx.status;
}

static uint16_t ring_next_index(uint16_t index)
{
    return (uint16_t)((index + 1U) % VISION_RING_BUFFER_SIZE);
}

static uint16_t ring_count(void)
{
    if (vision_input_ctx.ring_head >= vision_input_ctx.ring_tail)
    {
        return (uint16_t)(vision_input_ctx.ring_head - vision_input_ctx.ring_tail);
    }

    return (uint16_t)(VISION_RING_BUFFER_SIZE - vision_input_ctx.ring_tail +
                      vision_input_ctx.ring_head);
}

static void ring_push_bytes(const uint8_t *data, uint16_t len)
{
    uint16_t i;

    if (data == NULL)
    {
        return;
    }

    for (i = 0U; i < len; i++)
    {
        uint16_t next_head = ring_next_index(vision_input_ctx.ring_head);

        if (next_head == vision_input_ctx.ring_tail)
        {
            vision_input_ctx.ring_tail = ring_next_index(vision_input_ctx.ring_tail);
            vision_input_ctx.status.dropped_bytes++;
        }

        vision_input_ctx.ring_buf[vision_input_ctx.ring_head] = data[i];
        vision_input_ctx.ring_head = next_head;
    }
}

static uint8_t ring_peek(uint16_t offset, uint8_t *value)
{
    uint16_t count = ring_count();
    uint16_t index;

    if (value == NULL || offset >= count)
    {
        return 0U;
    }

    index = (uint16_t)((vision_input_ctx.ring_tail + offset) % VISION_RING_BUFFER_SIZE);
    *value = vision_input_ctx.ring_buf[index];
    return 1U;
}

static void ring_drop(uint16_t len)
{
    uint16_t count = ring_count();

    if (len > count)
    {
        len = count;
    }

    vision_input_ctx.ring_tail =
      (uint16_t)((vision_input_ctx.ring_tail + len) % VISION_RING_BUFFER_SIZE);
}

static uint8_t frame_checksum(const uint8_t *frame)
{
    uint8_t checksum = 0U;
    uint16_t i;

    for (i = 2U; i <= 6U; i++)
    {
        checksum ^= frame[i];
    }

    return checksum;
}

static void parse_ring_frames(void)
{
    while (ring_count() >= VISION_LEGACY_FRAME_SIZE)
    {
        uint8_t byte0;
        uint8_t byte1;
        uint8_t tail0;
        uint8_t tail1;
        uint8_t frame[VISION_ENHANCED_FRAME_SIZE];
        uint16_t i;

        if (!ring_peek(0U, &byte0) || byte0 != VISION_FRAME_HEAD_0)
        {
            ring_drop(1U);
            continue;
        }

        if (!ring_peek(1U, &byte1) || byte1 != VISION_FRAME_HEAD_1)
        {
            ring_drop(1U);
            continue;
        }

        if (ring_count() >= VISION_ENHANCED_FRAME_SIZE &&
            ring_peek(8U, &tail0) && ring_peek(9U, &tail1) &&
            tail0 == VISION_FRAME_TAIL_0 && tail1 == VISION_FRAME_TAIL_1)
        {
            // 增强帧带 seq/checksum，方便调参时检查丢帧和错帧；
            // 旧的 8 字节帧仍保留，兼容早期工具。
            for (i = 0U; i < VISION_ENHANCED_FRAME_SIZE; i++)
            {
                ring_peek(i, &frame[i]);
            }

            if (frame_checksum(frame) != frame[7])
            {
                vision_input_ctx.status.checksum_errors++;
            }
            else
            {
                if ((uint8_t)(vision_input_ctx.status.last_seq + 1U) != frame[2] &&
                    vision_input_ctx.status.parsed_frames > 0U)
                {
                    vision_input_ctx.status.sequence_errors++;
                }

                store_frame((uint16_t)(((uint16_t)frame[4] << 8) | frame[3]),
                            (uint16_t)(((uint16_t)frame[6] << 8) | frame[5]),
                            frame[2], frame[7], 1U);
            }

            ring_drop(VISION_ENHANCED_FRAME_SIZE);
            continue;
        }

        if (ring_count() >= VISION_LEGACY_FRAME_SIZE &&
            ring_peek(6U, &tail0) && ring_peek(7U, &tail1) &&
            tail0 == VISION_FRAME_TAIL_0 && tail1 == VISION_FRAME_TAIL_1)
        {
            for (i = 0U; i < VISION_LEGACY_FRAME_SIZE; i++)
            {
                ring_peek(i, &frame[i]);
            }

            store_frame((uint16_t)(((uint16_t)frame[3] << 8) | frame[2]),
                        (uint16_t)(((uint16_t)frame[5] << 8) | frame[4]),
                        (uint8_t)(vision_input_ctx.status.last_seq + 1U), 0U, 0U);
            ring_drop(VISION_LEGACY_FRAME_SIZE);
            continue;
        }

        if (ring_count() >= VISION_ENHANCED_FRAME_SIZE)
        {
            vision_input_ctx.status.frame_errors++;
            ring_drop(1U);
            continue;
        }

        break;
    }
}

static void store_frame(uint16_t x, uint16_t y, uint8_t seq, uint8_t checksum, uint8_t enhanced)
{
    __disable_irq();
    // 只保存最新目标，不排队回放旧坐标；高速运动时旧坐标堆积会直接造成滞后。
    vision_input_ctx.latest_frame.x = x;
    vision_input_ctx.latest_frame.y = y;
    vision_input_ctx.latest_frame.seq = seq;
    vision_input_ctx.latest_frame.checksum = checksum;
    vision_input_ctx.latest_frame.enhanced = enhanced;
    vision_input_ctx.latest_frame.tick = HAL_GetTick();
    vision_input_ctx.frame_ready = 1U;
    vision_input_ctx.status.last_seq = seq;
    vision_input_ctx.status.link_online = 1U;
    vision_input_ctx.status.parsed_frames++;
    __enable_irq();
}
