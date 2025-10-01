// Core/Src/u8g2_stm32_hal.c
#include "u8g2_stm32_hal.h"

// === Микросекундные задержки через DWT (STM32H7) ===
static void DWT_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    // Разблокировка LAR для H7 (если применимо)
    *(volatile uint32_t*)0xE0001FB0 = 0xC5ACCE55;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static inline void delay_us(uint32_t us) {
    uint32_t ticks = (HAL_RCC_GetHCLKFreq() / 1000000U) * us;
    uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < ticks) { __NOP(); }
}

void U8G2_HAL_Init(void) {
    // Безопасные уровни (подстрой под свою плату, но у тебя так и есть):
    // CS = 1 (неактивен), DC = 0, RST = 1 (выведен из reset)
    HAL_GPIO_WritePin(CS_GPIO_Port,  CS_Pin,  GPIO_PIN_SET);
    HAL_GPIO_WritePin(D_C_GPIO_Port, D_C_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET);

    DWT_Init();
}

// ===================== GPIO + DELAY =====================
uint8_t u8x8_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    (void)u8x8; (void)arg_ptr;
    switch (msg) {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            U8G2_HAL_Init();
            return 1;

        case U8X8_MSG_DELAY_MILLI:
            HAL_Delay(arg_int);
            return 1;

        case U8X8_MSG_DELAY_10MICRO:
            delay_us(10U * arg_int);
            return 1;

        case U8X8_MSG_DELAY_NANO:
            __NOP(); // можно игнорировать на H7
            return 1;

        case U8X8_MSG_GPIO_DC:
            HAL_GPIO_WritePin(D_C_GPIO_Port, D_C_Pin, arg_int ? GPIO_PIN_SET : GPIO_PIN_RESET);
            return 1;

        case U8X8_MSG_GPIO_CS:
            HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, arg_int ? GPIO_PIN_SET : GPIO_PIN_RESET);
            return 1;

        case U8X8_MSG_GPIO_RESET:
            HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, arg_int ? GPIO_PIN_SET : GPIO_PIN_RESET);
            return 1;
    }
    return 0;
}

// ===================== BYTE (SPI) =====================
// ВАЖНО: CS опускается ОДИН РАЗ в START_TRANSFER и поднимается в END_TRANSFER.
// Внутри SEND НИЧЕГО не трогаем с CS — просто льём данные.
uint8_t u8x8_byte_stm32_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    (void)u8x8;
    switch (msg) {
        case U8X8_MSG_BYTE_INIT:
            // SPI инициализирован CubeMX (MX_SPI2_Init)
            return 1;

        case U8X8_MSG_BYTE_SET_DC:
            HAL_GPIO_WritePin(D_C_GPIO_Port, D_C_Pin, arg_int ? GPIO_PIN_SET : GPIO_PIN_RESET);
            return 1;

        case U8X8_MSG_BYTE_START_TRANSFER:
            // CS = Low, крошечная пауза
            HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
            delay_us(1);
            return 1;

        case U8X8_MSG_BYTE_SEND:
            if (arg_int > 0) {
                if (HAL_SPI_Transmit(&hspi2, (uint8_t*)arg_ptr, (uint16_t)arg_int, HAL_MAX_DELAY) != HAL_OK) {
                    return 0;
                }
            }
            return 1;

        case U8X8_MSG_BYTE_END_TRANSFER:
            // крошечные паузы до/после подъёма CS помогают некоторым OLED
            delay_us(1);
            HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
            delay_us(1);
            return 1;
    }
    return 0;
}

// ===================== U8G2 High-level =====================
// ПОВОРОТ 180°: используем U8G2_R2 в конструкторе.
void u8g2_Init(u8g2_t *u8g2) {
    // Было: U8G2_R0 —> Стало: U8G2_R2 (180 градусов)
    u8g2_Setup_ssd1322_nhd_256x64_f(u8g2, U8G2_R2, u8x8_byte_stm32_hw_spi, u8x8_gpio_and_delay_stm32);

    // ВСТРОЕННАЯ инициализация u8g2 — НЕ ДОБАВЛЯЕМ свои «13 команд»!
    u8g2_InitDisplay(u8g2);     // отправляет init-последовательность на SSD1322
    u8g2_SetPowerSave(u8g2, 0); // включить панель
}

void u8g2_Demo(u8g2_t *u8g2) {
    // «Миг» экрана, как в твоём bare-metal тесте
    u8g2_ClearBuffer(u8g2);
    u8g2_SendBuffer(u8g2); // чёрный
    HAL_Delay(300);

    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawBox(u8g2, 0, 0, 256, 64);
    u8g2_SendBuffer(u8g2); // «белый» (макс. заливка)
    HAL_Delay(300);

    // Текст
    u8g2_ClearBuffer(u8g2);
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_SetFont(u8g2, u8g2_font_ncenB14_tr);
    u8g2_DrawStr(u8g2, 6, 22, "Privet, Azizbek!");
    u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
    u8g2_DrawStr(u8g2, 6, 40, "SSD1322 + u8g2 (SPI, no DMA)");
    u8g2_SendBuffer(u8g2);
}
