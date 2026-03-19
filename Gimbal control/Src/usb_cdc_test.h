#ifndef USB_CDC_TEST_H
#define USB_CDC_TEST_H

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USB_TEST_REQ_HEAD0        0xAAU
#define USB_TEST_REQ_HEAD1        0x55U
#define USB_TEST_REQ_CMD_PITCH    0x01U
#define USB_TEST_REQ_TAIL         0x96U

#define USB_TEST_ACK_HEAD0        0x5AU
#define USB_TEST_ACK_HEAD1        0xA5U
#define USB_TEST_ACK_CMD          0x81U
#define USB_TEST_ACK_TAIL         0x69U

#define USB_TEST_STATUS_OK        0x00U
#define USB_TEST_STATUS_CRC_ERROR 0x01U

#define USB_TEST_REQ_FRAME_SIZE   7U
#define USB_TEST_ACK_FRAME_SIZE   7U
#define USB_TEST_RING_BUFFER_SIZE 128U
#define USB_CDC_TEST_FORCE_ACK_ON_RX 0U

#pragma pack(push, 1)
typedef struct
{
    uint8_t head0;
    uint8_t head1;
    uint8_t cmd;
    int16_t pitch_deg;
    uint8_t crc8;
    uint8_t tail;
} usb_test_request_frame_t;

typedef struct
{
    uint8_t head0;
    uint8_t head1;
    uint8_t cmd;
    uint8_t seq;
    uint8_t status;
    uint8_t crc8;
    uint8_t tail;
} usb_test_ack_frame_t;
#pragma pack(pop)

typedef struct
{
    uint32_t rx_bytes;
    uint32_t parsed_ok_count;
    uint32_t ack_sent_count;
    uint32_t crc_error_count;
    uint32_t frame_error_count;
    uint32_t tx_busy_count;
    int16_t last_pitch_deg;
} usb_cdc_test_stats_t;

typedef struct
{
    uint32_t rx_callback_count;
    uint32_t rx_callback_bytes;
    uint32_t feed_forward_count;
    uint32_t forced_ack_count;
    uint32_t tx_ok_count;
    uint32_t tx_fail_count;
    uint32_t last_rx_len;
    uint32_t last_tx_len;
    int32_t last_tx_result;
    uint8_t last_forced_seq;
} usb_cdc_transport_diag_t;

typedef struct
{
    uint32_t heartbeat_attempt_count;
    uint32_t heartbeat_ok_count;
    uint32_t heartbeat_fail_count;
    uint32_t last_tick;
    int32_t last_send_result;
    int32_t last_tx_state;
} usb_cdc_heartbeat_diag_t;

typedef struct
{
    uint32_t echo_attempt_count;
    uint32_t echo_ok_count;
    uint32_t echo_fail_count;
    uint32_t echo_tx_busy_count;
    uint32_t last_echo_len;
    uint32_t last_rx_len;
    int32_t last_tx_state;
} usb_cdc_echo_diag_t;

void UsbCdcTest_Init(void);
void UsbCdcTest_FeedBytes(const uint8_t *data, uint16_t len);
const usb_cdc_test_stats_t *UsbCdcTest_GetStats(void);
const usb_cdc_transport_diag_t *UsbCdcTest_GetTransportDiag(void);
const usb_cdc_heartbeat_diag_t *UsbCdcTest_GetHeartbeatDiag(void);
const usb_cdc_echo_diag_t *UsbCdcTest_GetEchoDiag(void);
uint8_t UsbCdcTest_Crc8(const uint8_t *data, uint16_t len);
void UsbCdcTest_OnUsbReceive(uint16_t len);
void UsbCdcTest_OnFeedForward(uint16_t len);
void UsbCdcTest_OnTxResult(uint16_t len, int result);
int UsbCdcTest_SendFixedAck(uint8_t seq, uint8_t status);
void UsbCdcTest_HeartbeatTick(uint32_t tick_ms);
int UsbCdcTest_GetTxState(void);
int UsbCdcTest_EchoBytes(const uint8_t *data, uint16_t len);
int16_t UsbCdcTest_GetPitchTargetDeg(void);
float UsbCdcTest_GetPitchTargetRad(void);

// Temporary transport abstraction. When CubeMX USB CDC files exist, implement
// this in usbd_cdc_if.c (or another USB device transport file) to actually send.
int UsbCdcTest_SendBytes(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif
