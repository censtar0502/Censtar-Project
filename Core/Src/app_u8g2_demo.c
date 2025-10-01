// Core/Src/app_u8g2_demo.c
#include "app_u8g2_demo.h"

static u8g2_t u8g2;

void APP_U8G2_Init(void) {
    U8G2_HAL_Init(); // безопасные уровни + DWT
    u8g2_Init(&u8g2);
    u8g2_Demo(&u8g2);
}

void APP_U8G2_Loop(void) {
    // Можешь добавить анимацию/перерисовку тут.
    HAL_Delay(1000);
}
