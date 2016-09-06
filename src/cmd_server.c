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

int cmd_server_monitor_processes(ProcessList* l, Server* s) {
    fd_set set;
    while (!shutdown_drain && (process_list_init_fd_set(l, &set) || CONFIG->keep_running)) {
        FD_SET(s->fd, &set);
        int max = process_list_max_fd(l, -1);
        max = max > s->fd ? max : s->fd;
        if (-1 == select(max + 1, &set, NULL, NULL, NULL)) {
            if (EINTR == errno && !shutdown_drain) { continue; }
            perror("select");
            return -1;
        }
        server_incomming(s, &set, l);
        process_list_forward(l, &set);
    }
    return 0;
}

int cmd_server(int argc, char **argv) {
    int result = -1;
    l = config_read_drainfile(CONFIG->drainfile);
    Server s;
    server_init(&s);

    if (-1 == server_start(&s)) {
        fputs("could not start socket-server\n", stderr);
    } else if (!l && !CONFIG->keep_running) {
        fputs("no processes to start\n", stderr);
    } else {
        signal(SIGINT, cmd_server_stop);
        signal(SIGPIPE, cmd_server_sigpipe);
        signal(SIGINFO, cmd_server_siginfo);

        process_list_process_start(l, argc, argv);
        result = cmd_server_monitor_processes(l, &s);
    }

    server_stop(&s);
    process_list_free(l);

    return result;
}
