#include "usb_cdc_test.h"

#include <string.h>

typedef struct
{
    uint8_t ring[USB_TEST_RING_BUFFER_SIZE];
    uint16_t head;
    uint16_t tail;
    volatile int16_t pitch_target_deg;
    usb_cdc_test_stats_t stats;
    usb_cdc_transport_diag_t transport_diag;
    usb_cdc_heartbeat_diag_t heartbeat_diag;
    usb_cdc_echo_diag_t echo_diag;
} usb_cdc_test_context_t;

static usb_cdc_test_context_t g_usb_cdc_test = {0};

static uint16_t UsbCdcTest_RingCount(void);
static uint16_t UsbCdcTest_RingNext(uint16_t index);
static void UsbCdcTest_RingPush(const uint8_t *data, uint16_t len);
static uint8_t UsbCdcTest_RingPeek(uint16_t offset, uint8_t *value);
static void UsbCdcTest_RingDrop(uint16_t len);
static void UsbCdcTest_ParseFrames(void);

#define USB_TEST_DEG_TO_RAD 0.0174532925199432958f

void UsbCdcTest_Init(void)
{
    memset(&g_usb_cdc_test, 0, sizeof(g_usb_cdc_test));
}

const usb_cdc_test_stats_t *UsbCdcTest_GetStats(void)
{
    return &g_usb_cdc_test.stats;
}

const usb_cdc_transport_diag_t *UsbCdcTest_GetTransportDiag(void)
{
    return &g_usb_cdc_test.transport_diag;
}

const usb_cdc_heartbeat_diag_t *UsbCdcTest_GetHeartbeatDiag(void)
{
    return &g_usb_cdc_test.heartbeat_diag;
}

const usb_cdc_echo_diag_t *UsbCdcTest_GetEchoDiag(void)
{
    return &g_usb_cdc_test.echo_diag;
}

int16_t UsbCdcTest_GetPitchTargetDeg(void)
{
    return g_usb_cdc_test.pitch_target_deg;
}

float UsbCdcTest_GetPitchTargetRad(void)
{
    return (float)g_usb_cdc_test.pitch_target_deg * USB_TEST_DEG_TO_RAD;
}

uint8_t UsbCdcTest_Crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0x00U;
    uint16_t i;
    uint8_t bit;

    for (i = 0U; i < len; i++)
    {
        crc ^= data[i];
        for (bit = 0U; bit < 8U; bit++)
        {
            if (crc & 0x80U)
            {
                crc = (uint8_t)((crc << 1U) ^ 0x07U);
            }
            else
            {
                crc <<= 1U;
            }
        }
    }

    return crc;
}

void UsbCdcTest_FeedBytes(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0U)
    {
        return;
    }

    g_usb_cdc_test.stats.rx_bytes += len;
    UsbCdcTest_RingPush(data, len);
    UsbCdcTest_ParseFrames();
}

void UsbCdcTest_OnUsbReceive(uint16_t len)
{
    g_usb_cdc_test.transport_diag.rx_callback_count++;
    g_usb_cdc_test.transport_diag.rx_callback_bytes += len;
    g_usb_cdc_test.transport_diag.last_rx_len = len;
}

void UsbCdcTest_OnFeedForward(uint16_t len)
{
    g_usb_cdc_test.transport_diag.feed_forward_count++;
    g_usb_cdc_test.transport_diag.last_rx_len = len;
}

void UsbCdcTest_OnTxResult(uint16_t len, int result)
{
    g_usb_cdc_test.transport_diag.last_tx_len = len;
    g_usb_cdc_test.transport_diag.last_tx_result = result;

    if (result == (int)len)
    {
        g_usb_cdc_test.transport_diag.tx_ok_count++;
    }
    else
    {
        g_usb_cdc_test.transport_diag.tx_fail_count++;
    }
}

__weak int UsbCdcTest_SendBytes(const uint8_t *data, uint16_t len)
{
    (void)data;
    (void)len;
    return -1;
}

int UsbCdcTest_SendFixedAck(uint8_t seq, uint8_t status)
{
    usb_test_ack_frame_t ack;
    int ret;

    ack.head0 = USB_TEST_ACK_HEAD0;
    ack.head1 = USB_TEST_ACK_HEAD1;
    ack.cmd = USB_TEST_ACK_CMD;
    ack.seq = seq;
    ack.status = status;
    ack.crc8 = UsbCdcTest_Crc8(&ack.cmd, 3U);
    ack.tail = USB_TEST_ACK_TAIL;

    ret = UsbCdcTest_SendBytes((const uint8_t *)&ack, (uint16_t)sizeof(ack));
    UsbCdcTest_OnTxResult((uint16_t)sizeof(ack), ret);

    if (ret == (int)sizeof(ack))
    {
        g_usb_cdc_test.transport_diag.forced_ack_count++;
        g_usb_cdc_test.transport_diag.last_forced_seq = seq;
    }

    return ret;
}

__weak int UsbCdcTest_GetTxState(void)
{
    return -1;
}

void UsbCdcTest_HeartbeatTick(uint32_t tick_ms)
{
    static const uint8_t heartbeat_frame[] = {0x48U, 0x42U, 0x0DU, 0x0AU}; /* "HB\r\n" */
    usb_cdc_heartbeat_diag_t *diag = &g_usb_cdc_test.heartbeat_diag;
    int ret;

    if (tick_ms < 1000U)
    {
        return;
    }

    if (diag->last_tick != 0U && (tick_ms - diag->last_tick) < 500U)
    {
        return;
    }

    diag->last_tick = tick_ms;
    diag->heartbeat_attempt_count++;
    diag->last_tx_state = UsbCdcTest_GetTxState();

    ret = UsbCdcTest_SendBytes(heartbeat_frame, (uint16_t)sizeof(heartbeat_frame));
    diag->last_send_result = ret;

    if (ret == (int)sizeof(heartbeat_frame))
    {
        diag->heartbeat_ok_count++;
    }
    else
    {
        diag->heartbeat_fail_count++;
    }
}

int UsbCdcTest_EchoBytes(const uint8_t *data, uint16_t len)
{
    usb_cdc_echo_diag_t *diag = &g_usb_cdc_test.echo_diag;
    uint16_t echo_len;
    int tx_state;
    int ret;

    if (data == NULL || len == 0U)
    {
        return -1;
    }

    echo_len = (len > 16U) ? 16U : len;

    diag->echo_attempt_count++;
    diag->last_rx_len = len;
    diag->last_echo_len = echo_len;

    tx_state = UsbCdcTest_GetTxState();
    diag->last_tx_state = tx_state;

    if (tx_state != 0)
    {
        diag->echo_fail_count++;
        diag->echo_tx_busy_count++;
        return -1;
    }

    ret = UsbCdcTest_SendBytes(data, echo_len);
    if (ret == (int)echo_len)
    {
        diag->echo_ok_count++;
    }
    else
    {
        diag->echo_fail_count++;
    }

    return ret;
}

static uint16_t UsbCdcTest_RingNext(uint16_t index)
{
    return (uint16_t)((index + 1U) % USB_TEST_RING_BUFFER_SIZE);
}

static uint16_t UsbCdcTest_RingCount(void)
{
    if (g_usb_cdc_test.head >= g_usb_cdc_test.tail)
    {
        return (uint16_t)(g_usb_cdc_test.head - g_usb_cdc_test.tail);
    }

    return (uint16_t)(USB_TEST_RING_BUFFER_SIZE - g_usb_cdc_test.tail + g_usb_cdc_test.head);
}

static void UsbCdcTest_RingPush(const uint8_t *data, uint16_t len)
{
    uint16_t i;

    for (i = 0U; i < len; i++)
    {
        const uint16_t next_head = UsbCdcTest_RingNext(g_usb_cdc_test.head);
        if (next_head == g_usb_cdc_test.tail)
        {
            g_usb_cdc_test.tail = UsbCdcTest_RingNext(g_usb_cdc_test.tail);
        }

        g_usb_cdc_test.ring[g_usb_cdc_test.head] = data[i];
        g_usb_cdc_test.head = next_head;
    }
}

static uint8_t UsbCdcTest_RingPeek(uint16_t offset, uint8_t *value)
{
    uint16_t index;

    if (value == NULL || offset >= UsbCdcTest_RingCount())
    {
        return 0U;
    }

    index = (uint16_t)((g_usb_cdc_test.tail + offset) % USB_TEST_RING_BUFFER_SIZE);
    *value = g_usb_cdc_test.ring[index];
    return 1U;
}

static void UsbCdcTest_RingDrop(uint16_t len)
{
    uint16_t count = UsbCdcTest_RingCount();

    if (len > count)
    {
        len = count;
    }

    g_usb_cdc_test.tail = (uint16_t)((g_usb_cdc_test.tail + len) % USB_TEST_RING_BUFFER_SIZE);
}

static void UsbCdcTest_ParseFrames(void)
{
    while (UsbCdcTest_RingCount() >= USB_TEST_REQ_FRAME_SIZE)
    {
        usb_test_request_frame_t frame;
        uint8_t *raw = (uint8_t *)&frame;
        uint16_t i;
        uint8_t expected_crc;

        if (!UsbCdcTest_RingPeek(0U, &raw[0]) || raw[0] != USB_TEST_REQ_HEAD0)
        {
            UsbCdcTest_RingDrop(1U);
            continue;
        }

        if (!UsbCdcTest_RingPeek(1U, &raw[1]) || raw[1] != USB_TEST_REQ_HEAD1)
        {
            UsbCdcTest_RingDrop(1U);
            continue;
        }

        if (UsbCdcTest_RingCount() < USB_TEST_REQ_FRAME_SIZE)
        {
            break;
        }

        for (i = 0U; i < USB_TEST_REQ_FRAME_SIZE; i++)
        {
            UsbCdcTest_RingPeek(i, &raw[i]);
        }

        if (frame.tail != USB_TEST_REQ_TAIL || frame.cmd != USB_TEST_REQ_CMD_PITCH)
        {
            g_usb_cdc_test.stats.frame_error_count++;
            UsbCdcTest_RingDrop(1U);
            continue;
        }

        expected_crc = UsbCdcTest_Crc8(&frame.cmd, 3U);
        if (expected_crc != frame.crc8)
        {
            g_usb_cdc_test.stats.crc_error_count++;
            UsbCdcTest_RingDrop(USB_TEST_REQ_FRAME_SIZE);
            continue;
        }

        g_usb_cdc_test.pitch_target_deg = frame.pitch_deg;
        g_usb_cdc_test.stats.parsed_ok_count++;
        g_usb_cdc_test.stats.last_pitch_deg = frame.pitch_deg;
        UsbCdcTest_RingDrop(USB_TEST_REQ_FRAME_SIZE);
    }
}

