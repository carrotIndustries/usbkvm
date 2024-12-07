#pragma once
#include <stdint.h>

#define FLASH_HEADER_MAGIC (0x1337)

typedef struct {
    uint16_t magic;
    uint16_t app_version;
    uint32_t app_size;
    uint32_t app_crc;
    uint32_t header_crc;
} flash_header_t;

