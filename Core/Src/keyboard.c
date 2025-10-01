#include "keyboard.h"
#include <stdio.h>

// Расположение пинов столбцов и строк для удобства
#define ROW_NUM     4 // 4 ряда
#define COL_NUM     5 // 5 колонок

// Структура для удобного управления пинами
typedef struct {
    GPIO_TypeDef* port;
    uint16_t      pin;
} GPIO_Pin;

// Массив пинов-рядов (входы)
static const GPIO_Pin rows[ROW_NUM] = {
    {KeyRow1_GPIO_Port, KeyRow1_Pin},
    {KeyRow2_GPIO_Port, KeyRow2_Pin},
    {KeyRow3_GPIO_Port, KeyRow3_Pin},
    {KeyRow4_GPIO_Port, KeyRow4_Pin}
};

// Массив пинов-колонок (выходы)
static const GPIO_Pin cols[COL_NUM] = {
    {KeyCol1_GPIO_Port, KeyCol1_Pin},
    {KeyCol2_GPIO_Port, KeyCol2_Pin},
    {KeyCol3_GPIO_Port, KeyCol3_Pin},
    {KeyCol4_GPIO_Port, KeyCol4_Pin},
    {KeyCol5_GPIO_Port, KeyCol5_Pin}
};

// Символы на клавиатуре
static const char key_map[ROW_NUM][COL_NUM] = {
    {'1', '2', '3', 'A', 'B'},
    {'4', '5', '6', 'C', 'D'},
    {'7', '8', '9', 'E', 'F'},
    {'*', '0', '#', 'G', 'H'}
};

char KEYBOARD_Scan(void)
{
    char pressed_key = '\0'; // Возвращаемое значение по умолчанию

    for (int c = 0; c < COL_NUM; c++) // Идем по колонкам
    {
        // 1. Подаем LOW на текущую колонку
        HAL_GPIO_WritePin(cols[c].port, cols[c].pin, GPIO_PIN_RESET);

        // 2. Читаем все ряды
        for (int r = 0; r < ROW_NUM; r++)
        {
            if (HAL_GPIO_ReadPin(rows[r].port, rows[r].pin) == GPIO_PIN_RESET)
            {
                // Кнопка на пересечении (r, c) нажата!
                HAL_Delay(50); // Простое подавление дребезга контактов
                pressed_key = key_map[r][c];
                break; // Выходим из цикла рядов, т.к. кнопку уже нашли
            }
        }

        // 3. Возвращаем HIGH на текущую колонку
        HAL_GPIO_WritePin(cols[c].port, cols[c].pin, GPIO_PIN_SET);

        if (pressed_key != '\0')
        {
            break; // Выходим из цикла колонок
        }
    }
    return pressed_key;
}
