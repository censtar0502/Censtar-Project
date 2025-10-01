/* File: Core/Inc/logger.h */
#ifndef LOGGER_H_
#define LOGGER_H_

#include "main.h"
#include <stddef.h>
#include <stdint.h>

/* Системный лог (USART1) — для статусов системы, ошибок, printf и т.п. */
void Log_System(const char* fmt, ...);

/* Протокольный лог (USART2): печать кадра в hex с таймстампом.
   Пример: [t=123 ms][TRK-2][RX] 02 00 02 53 31 30 50 */
void Log_Frame(const char* direction, uint8_t trk_num, const uint8_t* frame, size_t length);

/* Байт-уровневый лог (USART2): печать одного байта RX/TX с таймстампом.
   Пример: [t=123 ms][TRK-2][RXb] AA */
void Log_Byte(const char* direction, uint8_t trk_num, uint8_t byte);

/* Печать произвольной строки в протокол-лог (USART2). */
void Log_Proto(const char* fmt, ...);

#endif /* LOGGER_H_ */
