#pragma once

#include <sys/un.h>
#include <sys/select.h>

typedef struct Server Server;

struct Server {
    int fd;
    struct sockaddr_un addr;
    /* struct sockaddr_in addr; */
    /* int port; */
};

void server_init(Server *s);

int server_start(Server *s);

void server_stop(Server *s);

int server_incomming(Server *s, fd_set *set);
