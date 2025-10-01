#include "gkl_frame.h"
#include <stddef.h> // Для NULL

#define GKL_SYN          0x02u // Стартовый байт
#define GKL_ADDR_HI      0x00u // Старший байт адреса всегда 0x00
#define GKL_MAX_DATA     22u   // Максимальная длина поля данных

uint8_t gkl_checksum_xor(const uint8_t* bytes, size_t len) {
    uint8_t xor_sum = 0u;
    for (size_t i = 0; i < len; ++i) {
        xor_sum ^= bytes[i];
    }
    return xor_sum;
}

size_t gkl_build_frame(uint8_t slave_addr, uint8_t cmd,
                       const uint8_t* data, size_t data_len,
                       uint8_t* out, size_t cap) {
    // Проверка входных данных
    if (!out || slave_addr < 1 || slave_addr > 32 || data_len > GKL_MAX_DATA) {
        return 0u;
    }

    // Проверка размера буфера
    // Структура: SYN(1) + ADDR(2) + CMD(1) + DATA(data_len) + XOR(1)
    size_t required_size = 5 + data_len;
    if (cap < required_size) {
        return 0u;
    }

    size_t index = 0;
    out[index++] = GKL_SYN;
    out[index++] = GKL_ADDR_HI;
    out[index++] = slave_addr;
    out[index++] = cmd;

    for (size_t i = 0; i < data_len; ++i) {
        out[index++] = data[i];
    }

    // Рассчитываем XOR для данных, начиная с ADDR_HI (индекс 1)
    // Длина данных для XOR = (общая длина - SYN) - сам байт XOR
    size_t checksum_data_len = required_size - 2;
    uint8_t checksum = gkl_checksum_xor(&out[1], checksum_data_len);
    out[index++] = checksum;

    return index; // Возвращаем общую длину кадра
}
