#ifndef GKL_FRAME_H_
#define GKL_FRAME_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Рассчитывает 1-байтовую XOR контрольную сумму.
 * @param bytes Указатель на данные.
 * @param len Длина данных.
 * @return 8-битная XOR сумма.
 */
uint8_t gkl_checksum_xor(const uint8_t* bytes, size_t len);

/**
 * @brief Собирает полный кадр команды для отправки в ТРК.
 * @param slave_addr Адрес ТРК (1-32).
 * @param cmd ASCII-код команды ('S', 'B', и т.д.).
 * @param data Указатель на данные команды (может быть NULL, если нет данных).
 * @param data_len Длина данных.
 * @param out_frame Буфер для записи собранного кадра.
 * @param out_capacity Размер выходного буфера.
 * @return Длина собранного кадра в байтах или 0 в случае ошибки.
 */
size_t gkl_build_frame(uint8_t slave_addr,
                       uint8_t cmd,
                       const uint8_t* data,
                       size_t data_len,
                       uint8_t* out_frame,
                       size_t out_capacity);

#endif /* GKL_FRAME_H_ */
