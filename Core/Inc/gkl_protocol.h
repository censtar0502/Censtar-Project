#ifndef GKL_PROTOCOL_H_
#define GKL_PROTOCOL_H_

#include "main.h"

/**
 * @brief Инициализирует модуль протокола, связывая его с UART-портами и их буферами.
 * @param huart_trk1 Указатель на UART_HandleTypeDef для нечетных адресов ТРК.
 * @param rx_buf1 Указатель на буфер приема для huart_trk1.
 * @param huart_trk2 Указатель на UART_HandleTypeDef для четных адресов ТРК.
 * @param rx_buf2 Указатель на буфер приема для huart_trk2.
 */
void GKL_Protocol_Init(UART_HandleTypeDef* huart_trk1, uint8_t* rx_buf1,
                       UART_HandleTypeDef* huart_trk2, uint8_t* rx_buf2);

/**
 * @brief Отправляет команду запроса статуса.
 * @param slave_addr Адрес ТРК.
 */
void GKL_SendStatusRequest(uint8_t slave_addr);

/**
 * @brief Отправляет команду "СТОП".
 * @param slave_addr Адрес ТРК.
 */
void GKL_SendStop(uint8_t slave_addr);


#endif /* GKL_PROTOCOL_H_ */
