#include "config.h"
#include "process_list.h"
#include "server.h"
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "client")) {
        Client c;
        client_init(&c);
        client_start(&c);
        return EXIT_SUCCESS;
    }

    Server s;
    server_init(&s);
    server_start(&s);

    signal(SIGWINCH, config_init_term_width);
    config_init();
    ProcessList *l = config_read(CONFIG->drainfile);
    if (!l) {
        fputs("No processes to start\n", stderr);
        return EXIT_FAILURE;
    }

    process_list_process_start(l, argc - 1, argv + 1);

    fd_set set;
    int max = process_list_max_fd(l, -1);
    max = max > s.fd ? max : s.fd;
    while (process_list_init_fd_set(l, &set)) {
        FD_SET(s.fd, &set);
        if (-1 == select(max + 1, &set, NULL, NULL, NULL) && EINTR != errno) {
            perror("select");
            return EXIT_FAILURE;
        }
        server_incomming(&s, &set);
        l = process_list_forward(l, &set);
    }

    server_stop(&s);
    process_list_free(l);

    return EXIT_SUCCESS;
}
