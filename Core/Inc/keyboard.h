#ifndef __KEYBOARD_H
#define __KEYBOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

// Функция для сканирования клавиатуры.
// Возвращает символ нажатой кнопки или '\0' (ноль), если ничего не нажато.
char KEYBOARD_Scan(void);

#ifdef __cplusplus
}
#endif

#endif /* __KEYBOARD_H */
