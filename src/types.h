#pragma once

#include <stdint.h>

typedef struct ProcessList ProcessList;
typedef struct Server Server;

#define MAX_MESSAGE_CONTENT_SIZE 4096

typedef struct {
    int16_t nr;
    uint16_t size;
    char content[MAX_MESSAGE_CONTENT_SIZE];
} Message;
