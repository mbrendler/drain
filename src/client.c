#include "client.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>

int client_do(const Message* out, Message* in) {
    Client c;
    client_init(&c);
    if (-1 == client_start(&c)) { return -1; }
    if (-1 == client_send(&c, out)) { return -1; }
    if (-1 == client_receive(&c, in)) { return -1; }
    client_stop(&c);
    return 0;
}

void client_init(Client *c) {
    c->fd = -1;
}

int client_start(Client *c) {
    /* c->fd = socket(AF_INET, SOCK_STREAM, 0); */
    c->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == c->fd) {
        perror("socket");
        return -1;
    }

    /* struct in_addr ipv4addr; */
    /* inet_pton(AF_INET, "127.0.0.1", &ipv4addr); */
    /* struct hostent *server = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET); */
    /* if (server == NULL) { */
    /*     client_stop(c); */
    /*     perror("hostent"); */
    /*     return -1; */
    /* } */

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, CONFIG->socket_path, sizeof(addr.sun_path) - 1);
    /* struct sockaddr_in addr; */
    /* bzero((char *) &addr, sizeof(addr)); */
    /* addr.sin_family = AF_INET; */
    /* bcopy( */
    /*     (char *)server->h_addr, */
    /*     (char *)&addr.sin_addr.s_addr, */
    /*     server->h_length */
    /* ); */
    /* addr.sin_port = htons(9999); */

    if (-1 == connect(c->fd, (struct sockaddr*)&addr, sizeof(addr))) {
        client_stop(c);
        perror("connect");
        return -1;
    }

    return 0;
}

int client_send(Client *c, const Message* out) {
    if (-1 == write(c->fd, &out->nr, sizeof(out->nr))) {
        client_stop(c);
        perror("write nr");
        return -1;
    }
    if (-1 == write(c->fd, &out->size, sizeof(out->size))) {
        client_stop(c);
        perror("write size");
        return -1;
    }
    if (-1 == write(c->fd, &out->content, out->size)) {
        client_stop(c);
        perror("write content");
        return -1;
    }
    return 0;
}

int client_receive(Client *c, Message* in) {
    if (-1 == read(c->fd, &in->nr, sizeof(in->nr))) {
        client_stop(c);
        perror("read nr");
        return -1;
    }
    if (-1 == read(c->fd, &in->size, sizeof(in->size))) {
        client_stop(c);
        perror("read size");
        return -1;
    }
    if (-1 == read(c->fd, &in->content, in->size)) {
        client_stop(c);
        perror("read content");
        return -1;
    }
    return 0;
}

void client_stop(Client *c) {
    if (-1 == close(c->fd)) {
        perror("client close");
    }
    c->fd = -1;
}
