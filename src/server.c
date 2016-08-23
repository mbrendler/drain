#include "server.h"
#include "actions.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

void server_init(Server *s) {
    s->fd = -1;
    s->port = 9999;
}

int server_start(Server *s) {
    s->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == s->fd) {
        perror("socket");
        return -1;
    }

    s->addr.sin_family = AF_INET;
    s->addr.sin_port = htons(s->port);
    s->addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(s->addr.sin_zero), 8);

    if (-1 == setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt");
        return -1;
    }

    if (-1 == bind(s->fd, (struct sockaddr *)&s->addr, sizeof(struct sockaddr))) {
        perror("bind");
        return -1;
    }

    if (-1 == listen(s->fd, 2)) {
        perror("listen");
        return -1;
    }

    if (-1 == fcntl(s->fd, F_SETFL, fcntl(s->fd, F_GETFL, 0) | O_NONBLOCK)) {
        perror("fcntl");
        return -1;
    }
    return 0;
}

void server_stop(Server *s) {
    close(s->fd);
    s->fd = 0;
}

int server_incomming(Server *s, fd_set *set, ProcessList *l) {
    if (s->fd < 0 || !FD_ISSET(s->fd, set)) { return 0; }
    struct sockaddr_in addr;
    socklen_t sin_size;
    int fd = accept(s->fd, (struct sockaddr *)&addr, &sin_size);
    if (-1 == fd) {
        perror("accept");
        return -1;
    }
    puts("=================== connected ============================");

    if (-1 == fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK)) {
        close(fd);
        perror("fcntl");
        return -1;
    }

    Message in;
    if (-1 == recv(fd, &in.nr, sizeof(in.nr), 0)) {
        close(fd);
        perror("recv nr");
        return -1;
    }
    if (-1 == recv(fd, &in.size, sizeof(in.size), 0)) {
        close(fd);
        perror("recv size");
        return -1;
    }
    if (-1 == recv(fd, &in.content, in.size, 0)) {
        close(fd);
        perror("recv content");
        return -1;
    }
    Message out;
    if (-1 == perform_action(&in, &out, l)) {
        close(fd);
        fprintf(stderr, "perform_action\n");
        return -1;
    }
    if (-1 == send(fd, &out.nr, sizeof(out.nr), 0)) {
        close(fd);
        perror("send nr");
        return -1;
    }
    if (-1 == send(fd, &out.size, sizeof(out.size), 0)) {
        close(fd);
        perror("send size");
        return -1;
    }
    if (-1 == send(fd, &out.content, out.size, 0)) {
        close(fd);
        perror("send content");
        return -1;
    }

    close(fd);
    return 0;
}
