/* File: Core/Src/main.c */
/* 01.02.2025*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

/* =========================
 *  Константы протокола GKL
 * ========================= */
#define GKL_STX              0x02u
#define GKL_FIXED_FRAME_LEN  7u   /* 02 00 ADDR 'S' '1' '0' STATUS */
#define GKL_ADDR_TRK1        0x01u
#define GKL_ADDR_TRK2        0x02u

/* Команда опроса (последний байт запроса) — по вашим логам:
 * TRK-1: 'R' (0x52), TRK-2: 'Q' (0x51) */
#define GKL_CMD_POLL_TRK1    0x52u /* 'R' */
#define GKL_CMD_POLL_TRK2    0x51u /* 'Q' */

/* Тайминги */
#define POLL_INTERVAL_MS     200u  /* период опроса каждого ТРК */
#define REPLY_TIMEOUT_MS      80u  /* сколько ждём ответ после запроса по КАЖДОМУ порту */
#define INTERBYTE_GAP_RESET_MS  10u/* если межбайтовый разрыв > X мс, сбрасываем сборку кадра */

/* =========================
 *  Утилиты логирования
 * ========================= */

/* Лог «системы» — USART1 */
static void Log_System(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    HAL_UART_Transmit(&huart1, (uint8_t*)buf, (uint16_t)strlen(buf), 200);
}

/* Лог «протокола/двух ТРК» — USART2 */
static void Log_Proto(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    HAL_UART_Transmit(&huart2, (uint8_t*)buf, (uint16_t)strlen(buf), 200);
}

/* Красиво вывести массив байт в hex (для логов) */
static const char* hex_buf_to_string(const uint8_t *data, uint16_t len)
{
    static char line[3 * 64 + 4]; /* до 64 байт на строку */
    uint16_t n = (len > 64) ? 64 : len;
    uint16_t pos = 0;
    for (uint16_t i = 0; i < n; ++i)
    {
        int written = snprintf(&line[pos], sizeof(line) - pos, "%02X ", data[i]);
        if (written < 0) break;
        pos += (uint16_t)written;
        if (pos >= sizeof(line) - 1) break;
    }
    line[pos] = '\0';
    return line;
}

/* Вывод одного байта (RXb/TXb) с отметкой времени */
static void Log_Byte(const char* tag, const char* trk_tag, uint8_t b)
{
    uint32_t t = HAL_GetTick();
    Log_Proto("[t=%lu ms][%s][%s] %02X\r\n", (unsigned long)t, trk_tag, tag, (unsigned)b);
}

/* =========================
 *  Простейший парсер-буфер
 * ========================= */
typedef struct {
    uint8_t  buf[GKL_FIXED_FRAME_LEN];
    uint8_t  idx;              /* сколько уже набрали */
    uint32_t last_byte_ms;     /* для межбайтового контроля */
} gkl_parser_t;

static void Parser_Reset(gkl_parser_t *p)
{
    p->idx = 0;
    p->last_byte_ms = 0;
}

static bool Parser_IsComplete(const gkl_parser_t *p)
{
    return (p->idx == GKL_FIXED_FRAME_LEN);
}

/* Приходит один байт RX — собираем кадр фиксированной длины */
static void Parser_OnByte(gkl_parser_t *p, uint8_t b)
{
    uint32_t now = HAL_GetTick();

    /* Межбайтовый разрыв — сброс сборки */
    if (p->idx > 0 && (now - p->last_byte_ms) > INTERBYTE_GAP_RESET_MS)
    {
        p->idx = 0;
    }
    p->last_byte_ms = now;

    if (p->idx == 0)
    {
        if (b == GKL_STX)
        {
            p->buf[p->idx++] = b;
        }
        else
        {
            /* игнор до STX */
            return;
        }
    }
    else
    {
        if (p->idx < GKL_FIXED_FRAME_LEN)
        {
            p->buf[p->idx++] = b;
        }
        /* если превысили — защитный сброс */
        if (p->idx > GKL_FIXED_FRAME_LEN)
        {
            p->idx = 0;
        }
    }
}

/* =========================
 *  FSM для каждого порта
 * ========================= */
typedef enum {
    PORT_IDLE = 0,   /* можно отправлять новый запрос */
    PORT_WAIT_RX,    /* запрос отправлен, ждём полный ответ */
} port_state_t;

typedef struct {
    UART_HandleTypeDef *huart;
    const char         *tag;     /* "TRK-1" / "TRK-2" */
    uint8_t             addr;    /* 0x01 / 0x02 */
    uint8_t             poll_cmd;/* 'R' или 'Q' */
    port_state_t        state;
    uint32_t            t_next_poll_ms; /* когда можно инициировать следующий опрос */
    uint32_t            t_deadline_ms;  /* таймаут ожидания ответа на текущий запрос */
    gkl_parser_t        parser;
    uint8_t             rx_it_byte;     /* буфер для приёма по 1 байту в IT */
} trk_port_t;

/* Глобальные порты */
static trk_port_t TRK1;
static trk_port_t TRK2;

/* =========================
 *  Передача запроса
 * ========================= */
static HAL_StatusTypeDef TRK_SendPoll(trk_port_t *port)
{
    /* Кадр запроса по вашим логам: 02 00 ADDR 'S' CMD */
    uint8_t out[5];
    out[0] = GKL_STX;
    out[1] = 0x00;
    out[2] = port->addr;
    out[3] = 'S';
    out[4] = port->poll_cmd; /* 'R' для TRK-1, 'Q' для TRK-2 */

    /* Лог отправки */
    Log_Proto("[t=%lu ms][%s][TX] %s\r\n",
              (unsigned long)HAL_GetTick(), port->tag,
              hex_buf_to_string(out, sizeof(out)));

    return HAL_UART_Transmit(port->huart, out, sizeof(out), 50);
}

/* Когда парсер накопил 7 байт — проверяем «структуру» и логируем */
static void TRK_HandleCompleteFrame(trk_port_t *port)
{
    const uint8_t *b = port->parser.buf;

    /* Базовая структурная проверка кадра */
    bool looks_ok = (b[0] == GKL_STX) &&
                    (b[1] == 0x00)    &&
                    (b[2] == port->addr) &&
                    (b[3] == 'S')     &&
                    (b[4] == '1')     &&
                    (b[5] == '0');

    if (looks_ok)
    {
        Log_Proto("[t=%lu ms][%s][RX] %s\r\n",
                  (unsigned long)HAL_GetTick(), port->tag,
                  hex_buf_to_string(b, GKL_FIXED_FRAME_LEN));
    }
    else
    {
        Log_Proto("[t=%lu ms][%s][RX-BAD] %s\r\n",
                  (unsigned long)HAL_GetTick(), port->tag,
                  hex_buf_to_string(b, GKL_FIXED_FRAME_LEN));
    }
}

/* =========================
 *  Callbacks UART (RX IT)
 * ========================= */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == TRK1.huart)
    {
        uint8_t b = TRK1.rx_it_byte;
        Log_Byte("RXb", TRK1.tag, b);
        Parser_OnByte(&TRK1.parser, b);
        /* Переармирование приёма по байту */
        HAL_UART_Receive_IT(TRK1.huart, &TRK1.rx_it_byte, 1);
    }
    else if (huart == TRK2.huart)
    {
        uint8_t b = TRK2.rx_it_byte;
        Log_Byte("RXb", TRK2.tag, b);
        Parser_OnByte(&TRK2.parser, b);
        HAL_UART_Receive_IT(TRK2.huart, &TRK2.rx_it_byte, 1);
    }
}

/* Опциональная диагностика ошибок UART — в системный лог */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    uint32_t err = HAL_UART_GetError(huart);
    if (huart == &huart3)
        Log_System("UART3 error: 0x%08lX\r\n", (unsigned long)err);
    else if (huart == &huart6)
        Log_System("UART6 error: 0x%08lX\r\n", (unsigned long)err);
}

/* =========================
 *  Инициализация портов
 * ========================= */
static void TRK_InitPorts(void)
{
    /* TRK-1 на USART3 */
    TRK1.huart         = &huart3;
    TRK1.tag           = "TRK-1";
    TRK1.addr          = GKL_ADDR_TRK1;
    TRK1.poll_cmd      = GKL_CMD_POLL_TRK1; /* 'R' */
    TRK1.state         = PORT_IDLE;
    TRK1.t_next_poll_ms= 0;
    TRK1.t_deadline_ms = 0;
    Parser_Reset(&TRK1.parser);
    HAL_UART_Receive_IT(TRK1.huart, &TRK1.rx_it_byte, 1);

    /* TRK-2 на USART6 */
    TRK2.huart         = &huart6;
    TRK2.tag           = "TRK-2";
    TRK2.addr          = GKL_ADDR_TRK2;
    TRK2.poll_cmd      = GKL_CMD_POLL_TRK2; /* 'Q' */
    TRK2.state         = PORT_IDLE;
    TRK2.t_next_poll_ms= 0;
    TRK2.t_deadline_ms = 0;
    Parser_Reset(&TRK2.parser);
    HAL_UART_Receive_IT(TRK2.huart, &TRK2.rx_it_byte, 1);
}

/* Шаг FSM для одного порта */
static void TRK_FSM_Step(trk_port_t *port)
{
    uint32_t now = HAL_GetTick();

    switch (port->state)
    {
        case PORT_IDLE:
            if (now >= port->t_next_poll_ms)
            {
                /* отправляем новый запрос и переходим в ожидание ответа */
                if (TRK_SendPoll(port) == HAL_OK)
                {
                    Parser_Reset(&port->parser);
                    port->t_deadline_ms  = now + REPLY_TIMEOUT_MS;
                    port->state          = PORT_WAIT_RX;
                }
                else
                {
                    /* ошибка TX — переназначим попытку */
                    port->t_next_poll_ms = now + POLL_INTERVAL_MS;
                }
            }
            break;

        case PORT_WAIT_RX:
            /* полный кадр? — обрабатываем */
            if (Parser_IsComplete(&port->parser))
            {
                TRK_HandleCompleteFrame(port);
                /* очищаем парсер и ставим следующий опрос по интервалу */
                Parser_Reset(&port->parser);
                port->t_next_poll_ms = now + POLL_INTERVAL_MS;
                port->state = PORT_IDLE;
            }
            /* таймаут ожидания ответа */
            else if (now >= port->t_deadline_ms)
            {
                Log_Proto("[t=%lu ms][%s][TIMEOUT] no full frame in %u ms\r\n",
                          (unsigned long)now, port->tag, (unsigned)REPLY_TIMEOUT_MS);
                Parser_Reset(&port->parser);
                port->t_next_poll_ms = now + POLL_INTERVAL_MS;
                port->state = PORT_IDLE;
            }
            break;

        default:
            port->state = PORT_IDLE;
            break;
    }
}

/* =========================
 *  Стандартные заготовки
 * ========================= */
void SystemClock_Config(void);
static void MPU_Config(void);

int main(void)
{
    MPU_Config();
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    MX_USART3_UART_Init();
    MX_USART6_UART_Init();
    MX_SPI2_Init();

    Log_System("System up.\r\n");

    TRK_InitPorts();

    /* Для наглядности: разные фазы начального опроса */
    TRK1.t_next_poll_ms = HAL_GetTick() + 10;    /* стартовать почти сразу */
    TRK2.t_next_poll_ms = HAL_GetTick() + 100;   /* со сдвигом, чтобы логи читались легче */

    /* Главный цикл */
    while (1)
    {
        TRK_FSM_Step(&TRK1);
        TRK_FSM_Step(&TRK2);
        /* Можно добавить лёгкий idle-delay, чтобы снизить нагрузку */
        HAL_Delay(1);
    }
}

void SystemClock_Config(void)
{
    /* Вставьте вашу реализацию из CubeMX (оставлена без изменений) */
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
    while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 5;
    RCC_OscInitStruct.PLL.PLLN = 192;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 5;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                                | RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) { Error_Handler(); }
}

static void MPU_Config(void)
{
    MPU_Region_InitTypeDef MPU_InitStruct = {0};
    HAL_MPU_Disable();
    MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
    MPU_InitStruct.Number           = MPU_REGION_NUMBER0;
    MPU_InitStruct.BaseAddress      = 0x0;
    MPU_InitStruct.Size             = MPU_REGION_SIZE_4GB;
    MPU_InitStruct.SubRegionDisable = 0x87;
    MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
    MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
    MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsShareable      = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) { }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) { (void)file; (void)line; }
#endif /* USE_FULL_ASSERT */
