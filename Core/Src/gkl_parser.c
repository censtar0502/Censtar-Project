#include "gkl_parser.h"
#include "gkl_frame.h" // Нужен для gkl_checksum_xor

// Таблица ожидаемой длины ПОЛЕЙ ДАННЫХ для каждого типа ответа
// Длина = Response data length из протокола
static size_t get_expected_data_len(uint8_t cmd) {
    switch (cmd) {
        case 'S': return 2;  // status + nozzle
        case 'L': return 10; // nozzle + id + status + ';' + volume(6)
        case 'R': return 10; // nozzle + id + status + ';' + money(6)
        case 'T': return 22; // ...
        case 'C': return 11; // ...
        case 'Z': return 6;  // ...
        case 'D': return 2;  // ...
        default:  return 0;
    }
}

void GKL_Parser_Init(GKL_ParserState* p) {
    p->state = PARSER_STATE_WAIT_SYN;
    p->idx = 0;
}

GKL_ParseStatus GKL_Parser_ConsumeByte(GKL_ParserState* p, uint8_t byte) {
    if (p->idx >= GKL_MAX_FRAME_SIZE) {
        GKL_Parser_Init(p);
        return PARSE_ERROR_BUFFER_OVERFLOW;
    }

    switch (p->state) {
        case PARSER_STATE_WAIT_SYN:
            if (byte == 0x02) {
                p->buffer[p->idx++] = byte;
                p->state = PARSER_STATE_WAIT_ADDR_HI;
            }
            break;

        case PARSER_STATE_WAIT_ADDR_HI:
            p->buffer[p->idx++] = byte;
            p->state = PARSER_STATE_WAIT_ADDR_LO;
            break;

        case PARSER_STATE_WAIT_ADDR_LO:
            p->buffer[p->idx++] = byte;
            p->parsed_frame.slave_addr = byte;
            p->state = PARSER_STATE_WAIT_CMD;
            break;

        case PARSER_STATE_WAIT_CMD:
            p->buffer[p->idx++] = byte;
            p->parsed_frame.cmd = byte;
            p->expected_len = get_expected_data_len(byte);
            p->state = (p->expected_len > 0) ? PARSER_STATE_WAIT_DATA : PARSER_STATE_WAIT_CHECKSUM;
            break;

        case PARSER_STATE_WAIT_DATA:
            p->buffer[p->idx++] = byte;
            if (p->idx == (4 + p->expected_len)) { // SYN + ADDR_HI + ADDR_LO + CMD + DATA
                p->state = PARSER_STATE_WAIT_CHECKSUM;
            }
            break;

        case PARSER_STATE_WAIT_CHECKSUM: {
            uint8_t received_checksum = byte;
            // Данные для XOR: ADDR_HI, ADDR_LO, CMD, DATA...
            size_t checksum_len = p->idx - 1; // Все байты в буфере, кроме SYN
            uint8_t calculated_checksum = gkl_checksum_xor(&p->buffer[1], checksum_len);

            if (received_checksum == calculated_checksum) {
                // Успех! Копируем данные в выходную структуру
                p->parsed_frame.data_len = p->expected_len;
                for (size_t i = 0; i < p->expected_len; ++i) {
                    p->parsed_frame.data[i] = p->buffer[4 + i];
                }
                GKL_Parser_Init(p); // Сброс для следующего кадра
                return PARSE_SUCCESS;
            } else {
                GKL_Parser_Init(p); // Сброс
                return PARSE_ERROR_CHECKSUM;
            }
        }
    }
    return PARSE_IN_PROGRESS;
}
