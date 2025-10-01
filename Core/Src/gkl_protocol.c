#include "gkl_protocol.h"
#include "gkl_frame.h"
#include "usart.h"
#include "logger.h"  // Log_Frame (USART2)

static UART_HandleTypeDef* trk1_huart = NULL;
static UART_HandleTypeDef* trk2_huart = NULL;

/* Мы больше НЕ храним внешние RX-буферы — приёмом управляет только main.c */

void GKL_Protocol_Init(UART_HandleTypeDef* huart_trk1, uint8_t* rx_buf1_unused,
                       UART_HandleTypeDef* huart_trk2, uint8_t* rx_buf2_unused)
{
    (void)rx_buf1_unused;
    (void)rx_buf2_unused;
    trk1_huart = huart_trk1;
    trk2_huart = huart_trk2;
}

/**
 * @brief Внутренняя отправка кадра на нужный порт.
 * ВАЖНО: не трогаем приём (не Abort/Receive_IT здесь), чтобы не терять SYN.
 */
static void send_frame_to_trk(uint8_t slave_addr, uint8_t* frame, size_t frame_len)
{
    if (frame == NULL || frame_len == 0) return;

    UART_HandleTypeDef* huart = (slave_addr % 2U != 0U) ? trk1_huart : trk2_huart;
    uint8_t trk_num = (slave_addr % 2U != 0U) ? 1U : 2U;

    if (huart == NULL) return;

    /* Протокольный лог: сырой TX-кадр (время печатает logger) */
    Log_Frame("TX", trk_num, frame, frame_len);

    /* Отправка (блокирующая) */
    (void)HAL_UART_Transmit(huart, frame, (uint16_t)frame_len, 100);
}

void GKL_SendStatusRequest(uint8_t slave_addr)
{
    uint8_t frame_buffer[16];
    size_t frame_len = gkl_build_frame(slave_addr, 'S', NULL, 0, frame_buffer, sizeof(frame_buffer));
    if (frame_len > 0U) {
        send_frame_to_trk(slave_addr, frame_buffer, frame_len);
    }
}

void GKL_SendStop(uint8_t slave_addr)
{
    uint8_t frame_buffer[16];
    size_t frame_len = gkl_build_frame(slave_addr, 'B', NULL, 0, frame_buffer, sizeof(frame_buffer));
    if (frame_len > 0U) {
        send_frame_to_trk(slave_addr, frame_buffer, frame_len);
    }
}
