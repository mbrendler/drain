#include "config.h"
#include "process_list.h"
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#include "actions.h"

int main(int argc, char **argv) {
    // TODO: remove client argument
    if (argc > 1 && !strcmp(argv[1], "client")) {
        const char *cmd = argc > 2 ? argv[2] : "status";
        return perform_command(cmd, argc > 2 ? argc - 3 : 0, argv + 3);
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
        server_incomming(&s, &set, l);
        l = process_list_forward(l, &set);
    }

    server_stop(&s);
    process_list_free(l);

    return EXIT_SUCCESS;
}
