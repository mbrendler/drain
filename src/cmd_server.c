#include "config.h"
#include "drainfile.h"
#include "process_list.h"
#include "server.h"
#include <stdio.h>
#include <errno.h>
#include <signal.h>

static int shutdown_drain = 0;
static ProcessList *list = NULL;

ProcessList* process_list() {
    return list;
}

void cmd_server_stop() {
    shutdown_drain = 1;
}

void cmd_server_sigpipe() {
    // ignore sigpipe
}

void cmd_server_siginfo() {
    process_list_each(p, list, { process_print_status(p); });
}

void cmd_server_register_signal_handlers() {
    signal(SIGINT, cmd_server_stop);
    signal(SIGPIPE, cmd_server_sigpipe);
#   ifdef linux
    signal(SIGUSR1, cmd_server_siginfo);
#   else
    signal(SIGINFO, cmd_server_siginfo);
#   endif
}

int cmd_server_monitor_processes(ProcessList* l, Server* s) {
    fd_set set;
    list = l;
    while (!shutdown_drain && (process_list_init_fd_set(l, &set) || CONFIG->keep_running)) {
        if (s) { FD_SET(s->fd, &set); }
        const int max = process_list_max_fd(l, s ? s->fd : -1);
        if (-1 == select(max + 1, &set, NULL, NULL, NULL)) {
            if (EINTR == errno && !shutdown_drain) { continue; }
            perror("select");
            return -1;
        }
        if (s) { server_incomming(s, &set); }
        process_list_forward(l, &set);
    }
    return 0;
}

int cmd_server(int argc, char **argv) {
    int result = -1;
    list = drainfile_read(CONFIG->drainfile);
    Server s;
    server_init(&s);

    if (-1 == server_start(&s)) {
        fputs("could not start socket-server\n", stderr);
    } else if (!list && !CONFIG->keep_running) {
        fputs("no processes to start\n", stderr);
    } else {
        cmd_server_register_signal_handlers();
        process_list_process_start(list, argc, argv);
        result = cmd_server_monitor_processes(list, &s);
    }

    server_stop(&s);
    process_list_free(list);
    list = NULL;

    return result;
}
