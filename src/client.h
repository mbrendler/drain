#pragma once

#include "types.h"

typedef struct Client Client;

struct Client {
    int fd;
};

int client_do(const Message* out, Message* in);

void client_init(Client *c);

int client_start(Client *c);

void client_stop(Client *c);

int client_send(Client *c, const Message* out, Message* in);
