#ifndef GKL_PARSER_H_
#define GKL_PARSER_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define GKL_MAX_FRAME_SIZE 32 // Максимальный размер кадра (с запасом)

// Структура для хранения разобранного кадра
typedef struct {
    uint8_t slave_addr;
    uint8_t cmd;
    uint8_t data[22];
    size_t  data_len;
} GKL_Frame;

// Состояния парсера
typedef enum {
    PARSER_STATE_WAIT_SYN,
    PARSER_STATE_WAIT_ADDR_HI,
    PARSER_STATE_WAIT_ADDR_LO,
    PARSER_STATE_WAIT_CMD,
    PARSER_STATE_WAIT_DATA,
    PARSER_STATE_WAIT_CHECKSUM
} GKL_ParserFSMState;

// Статус разбора
typedef enum {
    PARSE_IN_PROGRESS,
    PARSE_SUCCESS,
    PARSE_ERROR_CHECKSUM,
    PARSE_ERROR_TIMEOUT, // (пока не используется, но задел на будущее)
    PARSE_ERROR_BUFFER_OVERFLOW
} GKL_ParseStatus;

// Контекст (состояние) парсера
typedef struct {
    GKL_ParserFSMState state;
    uint8_t            buffer[GKL_MAX_FRAME_SIZE];
    size_t             idx;
    size_t             expected_len;
    GKL_Frame          parsed_frame;
} GKL_ParserState;


void GKL_Parser_Init(GKL_ParserState* p);
GKL_ParseStatus GKL_Parser_ConsumeByte(GKL_ParserState* p, uint8_t byte);

#endif /* GKL_PARSER_H_ */
