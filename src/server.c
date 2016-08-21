#include "server.h"
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

char BUFFER[4096];
char OUTBUFFER[4096];

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

#include "actions.h"

int server_incomming(Server *s, fd_set *set) {
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

    ssize_t size = recv(fd, BUFFER, sizeof(BUFFER), 0);
    printf("size: %d - %d\n", (int)size, fcntl(fd, F_GETFL, 0) & O_NONBLOCK);
    write(STDOUT_FILENO, BUFFER + 1, size - 1);
    unsigned action_nr = *BUFFER;
    int out_size;
    if (-1 == perform_action(action_nr, size - 1, BUFFER + 1, &out_size, OUTBUFFER)) {
        close(fd);
        fprintf(stderr, "perform_action\n");
        return -1;
    }
    if (-1 == send(fd, OUTBUFFER, out_size, 0)) {
        close(fd);
        perror("send");
        return -1;
    }

    close(fd);
    return 0;
}
