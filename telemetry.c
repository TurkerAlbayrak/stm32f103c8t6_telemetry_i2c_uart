/*
 * telemetry.c
 *
 * Cerceve olusturma ve UART uzerinden gonderme mantigi.
 */

#include "telemetry.h"
#include <string.h>

static UART_HandleTypeDef *tlm_uart;
static uint8_t packet_id = 0;
static uint32_t sent_count = 0;

static uint8_t last_frame[2 + 1 + 1 + sizeof(TelemetryPayload_t) + 1];
static uint8_t last_frame_len = 0;

#define UART_TIMEOUT 100

void Telemetry_Init(UART_HandleTypeDef *huart)
{
    tlm_uart = huart;
    packet_id = 0;
    sent_count = 0;
}

static uint8_t Telemetry_Checksum(uint8_t len, const uint8_t *payload)
{
    /* Basit XOR checksum: len byte'i + tum payload byte'lari uzerinden.
     * Hata duzeltme yapmaz, ama bit bozulmasini/gurultuyu yakalar -
     * radyo linki uzerinden (HC-12/LoRa) tasinirken paketi atmak icin yeterli. */
    uint8_t chk = len;
    for (uint8_t i = 0; i < len; i++)
        chk ^= payload[i];
    return chk;
}

HAL_StatusTypeDef Telemetry_Send(const TelemetryPayload_t *payload)
{
    uint8_t frame[2 + 1 + 1 + sizeof(TelemetryPayload_t) + 1];
    uint8_t idx = 0;
    uint8_t len = (uint8_t)sizeof(TelemetryPayload_t);

    frame[idx++] = TELEMETRY_SYNC_BYTE_1;
    frame[idx++] = TELEMETRY_SYNC_BYTE_2;
    frame[idx++] = packet_id;
    frame[idx++] = len;

    memcpy(&frame[idx], payload, len);
    idx += len;

    frame[idx++] = Telemetry_Checksum(len, (const uint8_t *)payload);

    memcpy(last_frame, frame, idx);
    last_frame_len = idx;

    HAL_StatusTypeDef status = HAL_UART_Transmit(tlm_uart, frame, idx, UART_TIMEOUT);

    if (status == HAL_OK)
    {
        sent_count++;
    }

    packet_id++; /* 255'ten sonra otomatik 0'a doner (uint8_t overflow) */
    return status;
}

uint32_t Telemetry_GetSentCount(void)
{
    return sent_count;
}

void Telemetry_GetLastFrame(const uint8_t **outBuf, uint8_t *outLen)
{
    *outBuf = last_frame;
    *outLen = last_frame_len;
}
