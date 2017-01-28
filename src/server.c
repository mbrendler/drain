#include "server.h"
#include "config.h"
#include "actions.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>

void server_init(Server *s) {
    s->fd = -1;
    memset(&s->addr, 0, sizeof(s->addr));
}

int server_start(Server *s) {
    s->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == s->fd) {
        perror("socket");
        return -1;
    }

    s->addr.sun_family = AF_UNIX;
    strncpy(s->addr.sun_path, CONFIG->socket_path, sizeof(s->addr.sun_path) -1);

    if (-1 == setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt");
        return -1;
    }

    if (-1 == bind(s->fd, (struct sockaddr *)&s->addr, sizeof(s->addr))) {
        *s->addr.sun_path = '\0';
        perror("bind");
        return -1;
    }

    if (-1 == chmod(s->addr.sun_path, S_IRWXU)) {
        perror("chmod");
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
    s->fd = -1;
    unlink(s->addr.sun_path);
}

int server_incomming(Server *s, fd_set *set) {
    if (s->fd < 0 || !FD_ISSET(s->fd, set)) { return 0; }
    struct sockaddr_in addr;
    socklen_t sin_size;
    int fd = accept(s->fd, (struct sockaddr *)&addr, &sin_size);
    if (-1 == fd) {
        perror("accept");
        return -1;
    }

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
    if (sizeof(in.content) < in.size) {
        close(fd);
        fprintf(stderr, "received content size");
        return -1;
    }
    if (-1 == recv(fd, &in.content, in.size, 0)) {
        close(fd);
        perror("recv content");
        return -1;
    }
    Message out;
    const int rc = perform_action(fd, &in, &out);
    if (-1 == rc) {
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

    if (0 == rc) {
        close(fd);
    } else if (-1 == fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK)) {
        close(fd);
        perror("fcntl 2");
        return -1;
    }
    return 0;
}
