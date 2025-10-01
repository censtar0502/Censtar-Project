/* File: Core/Src/logger.c */
#include "logger.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define SYS_LOG_UART    (&huart1)   /* USART1 — системный лог */
#define PROTO_LOG_UART  (&huart2)   /* USART2 — лог протокола */
#define LOG_BUFFER_SIZE 256

void Log_System(const char* fmt, ...)
{
    char buffer[LOG_BUFFER_SIZE];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (len <= 0) return;
    if (len > (int)sizeof(buffer)) len = (int)sizeof(buffer);
    HAL_UART_Transmit(SYS_LOG_UART, (uint8_t*)buffer, (uint16_t)len, 100);
}

void Log_Frame(const char* direction, uint8_t trk_num, const uint8_t* frame, size_t length)
{
    if (!direction || !frame || length == 0) return;

    uint32_t t = HAL_GetTick(); /* таймстамп в мс */
    char line[LOG_BUFFER_SIZE];
    size_t off = 0;

    off += (size_t)snprintf(line + off, sizeof(line) - off,
                            "[t=%lu ms][TRK-%u][%s] ",
                            (unsigned long)t, (unsigned)trk_num, direction);
    if (off >= sizeof(line)) off = sizeof(line) - 1;

    for (size_t i = 0; i < length; ++i)
    {
        if (off + 4 >= sizeof(line)) break;
        off += (size_t)snprintf(line + off, sizeof(line) - off, "%02X ", frame[i]);
    }

    if (off + 2 < sizeof(line))
    {
        line[off++] = '\r';
        line[off++] = '\n';
        line[off] = '\0';
    }
    else
    {
        line[sizeof(line)-2] = '\r';
        line[sizeof(line)-1] = '\n';
    }

    HAL_UART_Transmit(PROTO_LOG_UART, (uint8_t*)line, (uint16_t)off, 100);
}

void Log_Byte(const char* direction, uint8_t trk_num, uint8_t byte)
{
    if (!direction) return;
    uint32_t t = HAL_GetTick();
    char line[64];
    int n = snprintf(line, sizeof(line), "[t=%lu ms][TRK-%u][%sb] %02X\r\n",
                     (unsigned long)t, (unsigned)trk_num, direction, byte);
    if (n <= 0) return;
    if (n > (int)sizeof(line)) n = (int)sizeof(line);
    HAL_UART_Transmit(PROTO_LOG_UART, (uint8_t*)line, (uint16_t)n, 100);
}

void Log_Proto(const char* fmt, ...)
{
    char buffer[LOG_BUFFER_SIZE];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (len <= 0) return;
    if (len > (int)sizeof(buffer)) len = (int)sizeof(buffer);
    HAL_UART_Transmit(PROTO_LOG_UART, (uint8_t*)buffer, (uint16_t)len, 100);
}
