// Core/Inc/u8g2_stm32_hal.h
#ifndef U8G2_STM32_HAL_H_
#define U8G2_STM32_HAL_H_

#include "main.h"
#include "u8g2.h"
#include "spi.h"     // для extern hspi2
#include "gpio.h"    // для *_GPIO_Port и *_Pin

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Инициализация низкоуровневой HAL-обвязки (без запуска u8g2).
 * Выставляет безопасные уровни на CS/DC/RST и включает DWT для us-задержек.
 */
void U8G2_HAL_Init(void);

/**
 * Полная инициализация u8g2 для SSD1322 256x64 (SPI, full-buffer).
 * НИКАКИХ кастомных 13 команд — используем встроенную init-последовательность u8g2.
 */
void u8g2_Init(u8g2_t *u8g2);

/**
 * Простой демо-рисунок (для быстрой проверки после Init).
 */
void u8g2_Demo(u8g2_t *u8g2);

// Колбэки для u8g2 (если хочешь вызвать Setup вручную)
uint8_t u8x8_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8x8_byte_stm32_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

#ifdef __cplusplus
}
#endif
#endif /* U8G2_STM32_HAL_H_ */
