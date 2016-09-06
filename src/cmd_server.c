#include "config.h"
#include "process_list.h"
#include "server.h"
#include <stdio.h>
#include <errno.h>
#include <signal.h>

static int shutdown_drain = 0;
static ProcessList *l = NULL;

void cmd_server_stop() {
    shutdown_drain = 1;
}

void cmd_server_sigpipe() {
    // ignore sigpipe
}

void cmd_server_siginfo() {
    process_list_each(l, process_print_status);
}

int cmd_server(int argc, char **argv) {
    int result = 0;
    l = config_read_drainfile(CONFIG->drainfile);
    Server s;
    server_init(&s);
    if (-1 == server_start(&s)) {
        goto bailout;
    }

    signal(SIGINT, cmd_server_stop);
    signal(SIGPIPE, cmd_server_sigpipe);
    signal(SIGINFO, cmd_server_siginfo);
    if (!l) {
        fputs("No processes to start\n", stderr);
        result = -1;
        goto bailout;
    }

    process_list_process_start(l, argc, argv);

    fd_set set;
    while (!shutdown_drain && (process_list_init_fd_set(l, &set) || CONFIG->keep_running)) {
        FD_SET(s.fd, &set);
        int max = process_list_max_fd(l, -1);
        max = max > s.fd ? max : s.fd;
        if (-1 == select(max + 1, &set, NULL, NULL, NULL)) {
            if (EINTR == errno && !shutdown_drain) { continue; }
            perror("select");
            result = -1;
            goto bailout;
        }
        server_incomming(&s, &set, l);
        l = process_list_forward(l, &set);
    }

bailout:
    server_stop(&s);
    process_list_free(l);

    return result;
}
