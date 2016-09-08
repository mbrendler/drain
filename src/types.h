#pragma once

#include <stdint.h>

typedef struct ProcessList ProcessList;
typedef struct Server Server;

typedef struct {
    int16_t nr;
    uint16_t size;
    char content[4096];
} Message;
