#include "commands.h"
#include "client.h"
#include "helpers.h"
#include "actions.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>

int cmd_ping(int argc, char** argv) {
    Message pkg;
    char *content = argc > 0 ? *argv : "hallo";
    memcpy(pkg.content, content, strlen(content) + 1);
    pkg.size = strlen(content) + 1;
    pkg.nr = mnPing;

    struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL);
    if (-1 == client_do(&pkg, &pkg)) { return -1; }
    gettimeofday(&tv2, NULL);
    double time = (
        (double)(tv2.tv_usec - tv1.tv_usec) / 1000 +
        (double) (tv2.tv_sec - tv1.tv_sec) * 1000
    );
    printf("%d bytes time=%.3f ms (%s)\n", pkg.size, time, pkg.content);
    return 0;
}

int cmd_status(int argc, char** argv) {
    (void)argc;
    (void)argv;
    Message out = { mnStatus, 0, "" };
    Message in;
    if (-1 == client_do(&out, &in)) { return -1; }
    puts(in.content);
    return 0;
}

int cmd_up(int argc, char** argv) {
    Message out, in;
    out.nr = mnUp;
    out.size = serialize_string_array(argv, argc, out.content, sizeof(out.content));
    if (-1 == client_do(&out, &in)) { return -1; }
    return 0;
}

int cmd_down(int argc, char** argv) {
    Message out, in;
    out.nr = mnDown;
    out.size = serialize_string_array(argv, argc, out.content, sizeof(out.content));
    if (-1 == client_do(&out, &in)) { return -1; }
    return 0;
}

int cmd_restart(int argc, char** argv) {
    Message out, in;
    out.nr = mnRestart;
    out.size = serialize_string_array(argv, argc, out.content, sizeof(out.content));
    if (-1 == client_do(&out, &in)) { return -1; }
    return 0;
}

bool is_error(const Message* msg) {
    return msg->nr < 0;
}

void handle_error(const Message* msg) {
    if (-1 == msg->nr) {
        fprintf(stderr, "%s\n", msg->content);
    } else {
        fprintf(stderr, "Unknown error number: %d\n", msg->nr);
    }
}

int cmd_add(int argc, char** argv) {
    bool start = strcmp("-s", argv[0]) == 0;
    if (start) {
        argc--;
        argv++;
    }
    if (argc < 3) { return -1; }
    Message out, in;
    out.nr = mnAdd;
    out.size = serialize_string_array(argv, argc, out.content, sizeof(out.content));
    replace_char(
        out.content + strlen(argv[0]) + 1 + strlen(argv[1]) + 1,
        out.content + out.size - 1,
        0, ' '
    );
    if (-1 == client_do(&out, &in)) { return -1; }
    if (is_error(&in)) {
        handle_error(&in);
        return -1;
    }
    if (start) {
        const int len = strlen(argv[0]) + 1;
        memcpy(out.content, argv[0], len);
        out.size = len;
        out.nr = mnUp;
        if (-1 == client_do(&out, &in)) { return -1; }
    }
    return 0;
}

int shutdown_drain = 0;

void stop() {
    shutdown_drain = 1;
}

#include "config.h"
#include "process_list.h"
#include "server.h"
#include "commands.h"
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

int cmd_server(int argc, char **argv) {
    int result = 0;
    Server s;
    server_init(&s);
    server_start(&s);

    signal(SIGINT, stop);
    ProcessList *l = config_read(CONFIG->drainfile);
    if (!l) {
        fputs("No processes to start\n", stderr);
        result = -1;
        goto bailout;
    }

    process_list_process_start(l, argc, argv);

    fd_set set;
    while (!shutdown_drain && process_list_init_fd_set(l, &set)) {
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

int cmd_help(int argc, char** argv);

typedef int(*CommandFunction)(int, char**);

typedef struct {
    char const * const name;
    const CommandFunction fn;
    char const * const short_help;
} Command;

const Command COMMANDS[] = {
    { "up",      cmd_up,      "up [NAME ...]                 -- start one, more or all processes" },
    { "status",  cmd_status,  "status                        -- status of drain server" },
    { "server",  cmd_server,  "server [NAME ...]             -- start drain server" },
    { "restart", cmd_restart, "restart [NAME ...]            -- restart one, more or all processes" },
    { "ping",    cmd_ping,    "ping                          -- ping drain server" },
    { "help",    cmd_help,    "help                          -- show this help" },
    { "down",    cmd_down,    "down [NAME ...]               -- stop one, more or all processes" },
    { "add",     cmd_add,     "add NAME COLOR CMD [ARGS ...] -- add a new process (no start)" },
};

int cmd_help(int argc, char** argv) {
    (void)argc;
    (void)argv;
    puts("drain [CMD]\n");
    const Command *cmd = COMMANDS + sizeof(COMMANDS) / sizeof(*COMMANDS);
    while (COMMANDS != cmd) {
        --cmd;
        printf("  %s\n", cmd->short_help);
    }
    return 0;
}

int perform_command(const char* name, int argc, char** argv) {
    const Command *cmd = COMMANDS + sizeof(COMMANDS) / sizeof(*COMMANDS);
    const Command *found = NULL;
    const int name_len = strlen(name);
    bool ambiguous = false;
    while (COMMANDS != cmd) {
        --cmd;
        if (!strncmp(name, cmd->name, name_len)) {
            if (NULL == found) {
                found = cmd;
            } else if (false == ambiguous) {
                ambiguous = true;
                printf("command '%s' is ambiguous:\n", name);
                printf("  %s %s", found->name, cmd->name);
            } else {
                printf(" %s", cmd->name);
            }
        }
    }
    if (NULL == found) {
        printf("unknown command '%s'\n", name);
        return -1;
    } else if (false != ambiguous) {
        puts("");
        return -1;
    }
    found->fn(argc, argv);
    return 0;
}
