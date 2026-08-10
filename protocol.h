#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define MAX_NICKNAME_LEN 32

typedef enum {
    MSG_AUTH = 1,
    MSG_TEXT = 2
} msg_type_t;

#pragma pack(push, 1)
typedef struct {
    uint8_t type;       // 1 byte
    uint16_t length;    // 2 bytes
} msg_header_t;
#pragma pack(pop)

#endif