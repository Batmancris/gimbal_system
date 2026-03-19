#ifndef VISION_INPUT_H
#define VISION_INPUT_H

#include "main.h"
#include "usart.h"

#define VISION_DMA_BUFFER_SIZE       64U
#define VISION_RING_BUFFER_SIZE      256U
#define VISION_LEGACY_FRAME_SIZE     8U
#define VISION_ENHANCED_FRAME_SIZE   10U

#define VISION_FRAME_HEAD_0          0xFAU
#define VISION_FRAME_HEAD_1          0xFBU
#define VISION_FRAME_TAIL_0          0xFCU
#define VISION_FRAME_TAIL_1          0xFDU

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint8_t seq;
    uint8_t checksum;
    uint8_t enhanced;
    uint32_t tick;
} vision_input_frame_t;

typedef struct
{
    uint32_t rx_bytes;
    uint32_t parsed_frames;
    uint32_t dropped_bytes;
    uint32_t checksum_errors;
    uint32_t frame_errors;
    uint32_t sequence_errors;
    uint32_t last_rx_tick;
    uint8_t link_online;
    uint8_t last_seq;
} vision_input_status_t;

void VisionInput_StartReception(void);
void VisionInput_HandleIdleInterrupt(void);
void VisionInput_FeedBytes(const uint8_t *data, uint16_t len);
uint8_t VisionInput_FetchFrame(vision_input_frame_t *frame);
const vision_input_status_t *VisionInput_GetStatus(void);

#endif
