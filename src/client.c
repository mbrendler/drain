#include "client.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

void client_init(Client *c) {
    c->fd = -1;
}

int client_start(Client *c) {
    c->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == c->fd) {
        perror("socket");
        return -1;
    }

    struct in_addr ipv4addr;
    inet_pton(AF_INET, "127.0.0.1", &ipv4addr);
    struct hostent *server = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET);
    if (server == NULL) {
        close(c->fd);
        perror("hostent");
        return -1;
    }

    struct sockaddr_in serveraddr;
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy(
        (char *)server->h_addr,
        (char *)&serveraddr.sin_addr.s_addr,
        server->h_length
    );
    serveraddr.sin_port = htons(9999);

    if (-1 == connect(c->fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr))) {
        close(c->fd);
        perror("ERROR connecting");
        return -1;
    }

    const char *msg = "\0hallo\n";
    int n = write(c->fd, msg, strlen(msg + 1) + 2);
    if (n < 0) {
        close(c->fd);
        perror("write");
        return -1;
    }

    char BUFFER[4096];
    n = read(c->fd, BUFFER, sizeof(BUFFER));
    if (n < 0) {
        close(c->fd);
        perror("read");
        return -1;
    }
    printf("%s", BUFFER);
    close(c->fd);
    return 0;
}
