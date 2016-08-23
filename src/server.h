#pragma once

#include "types.h"
#include <netinet/in.h>
#include <sys/select.h>

typedef struct {
    int fd;
    struct sockaddr_in addr;
    int port;
} Server;

void server_init(Server *s);

int server_start(Server *s);

void server_stop(Server *s);

int server_incomming(Server *s, fd_set *set, ProcessList *l);
