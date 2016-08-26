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

int cmd_help(int argc, char** argv);

typedef int(*CommandFunction)(int, char**);

typedef struct {
    char const * const name;
    const CommandFunction fn;
    char const * const short_help;
} Command;

const Command COMMANDS[] = {
    { "add",     cmd_add,     "add NAME COLOR CMD [ARGS ...] -- add a new process (no start)" },
    { "down",    cmd_down,    "down [NAME ...]               -- stop one, more or all processes" },
    { "help",    cmd_help,    "help                          -- show this help" },
    { "ping",    cmd_ping,    "ping                          -- ping drain server" },
    { "restart", cmd_restart, "restart [NAME ...]            -- restart one, more or all processes" },
    { "status",  cmd_status,  "status                        -- status of drain server" },
    { "up",      cmd_up,      "up [NAME ...]                 -- start one, more or all processes" },
};

int cmd_help(int argc, char** argv) {
    (void)argc;
    (void)argv;
    puts("drain [CMD]\n\n");
    const Command *cmd = COMMANDS + sizeof(COMMANDS) / sizeof(*COMMANDS);
    while (COMMANDS != cmd) {
        --cmd;
        printf("  %s\n", cmd->short_help);
    }
    return 0;
}

int perform_command(const char* name, int argc, char** argv) {
    const Command *cmd = COMMANDS + sizeof(COMMANDS) / sizeof(*COMMANDS);
    while (COMMANDS != cmd) {
        --cmd;
        if (!strcmp(name, cmd->name)) {
            return cmd->fn(argc, argv);
        }
    }
    printf("unknown command '%s'\n", name);
    return -1;
}
