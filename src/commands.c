#include "commands.h"
#include "cmd_server.h"
#include "config.h"
#include "client.h"
#include "helpers.h"
#include "actions.h"
#include "process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>

int cmd_ping(int argc, char** argv) {
    Message msg;
    char *content = argc > 0 ? *argv : "hallo";
    memcpy(msg.content, content, strlen(content) + 1);
    msg.size = strlen(content) + 1;
    msg.nr = mnPing;

    struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL);
    if (-1 == client_do(&msg, &msg)) { return -1; }
    gettimeofday(&tv2, NULL);
    double time = (
        (double)(tv2.tv_usec - tv1.tv_usec) / 1000 +
        (double) (tv2.tv_sec - tv1.tv_sec) * 1000
    );
    printf("%d bytes time=%.3f ms (%s)\n", msg.size, time, msg.content);
    return 0;
}

int cmd_status(int argc, char** argv) {
    (void)argc;
    (void)argv;
    Message msg = {.nr=mnStatus, .size=0, .content=""};
    if (-1 == client_do(&msg, &msg)) { return -1; }
    int pos = 0;
    while (pos < msg.size) {
        Process p;
        pos += process_deserialize(msg.content + pos, &p);
        process_print_status(&p);
    }
    return 0;
}

int cmd_up(int argc, char** argv) {
    Message msg;
    msg.nr = mnUp;
    msg.size = serialize_string_array(argv, argc, msg.content, sizeof(msg.content));
    if (-1 == client_do(&msg, &msg)) { return -1; }
    return 0;
}

int cmd_halt(int argc, char** argv) {
    Message msg;
    msg.nr = mnDown;
    msg.size = serialize_string_array(argv, argc, msg.content, sizeof(msg.content));
    if (-1 == client_do(&msg, &msg)) { return -1; }
    return 0;
}

int cmd_restart(int argc, char** argv) {
    Message msg;
    msg.nr = mnRestart;
    msg.size = serialize_string_array(argv, argc, msg.content, sizeof(msg.content));
    if (-1 == client_do(&msg, &msg)) { return -1; }
    return 0;
}

#include "process_list.h"
#include "process.h"
#include <errno.h>
#include <fcntl.h>

int cmd_attach(int argc, char** argv) {
    Message out, in;
    out.nr = mnLog;
    ProcessList *l = NULL;
    for (int i = 0; i < argc; ++i) {
        out.size = strlen(argv[i]) + 1;
        strncpy(out.content, argv[i], sizeof(out.content));
        Client c;
        client_init(&c);
        if (-1 == client_start(&c)) { return -1; }
        if (-1 == client_send(&c, &out, &in)) { return -1; }
        if (0 != in.nr) {
            client_stop(&c);
            fprintf(stderr, "process not found - %s\n", argv[i]);
            continue;
        }
        if (-1 == fcntl(c.fd, F_SETFL, fcntl(c.fd, F_GETFL, 0) | O_NONBLOCK)) {
            perror("fcntl");
            return -1;
        }
        Process p;
        process_deserialize(in.content, &p);
        ProcessList *new = process_list_new(argv[i], p.cmd, p.color, c.fd);
        if (new) {
            l = process_list_append(l, &new);
        } else {
            fprintf(stderr, "ignore: %s\n", argv[i]);
        }
    }

    cmd_server_register_signal_handlers();
    const int result = cmd_server_monitor_processes(l, NULL);
    process_list_free(l);
    return result;
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

int cmd_drainfile(int argc, char** argv) {
    if (argc > 0 && 0 == strcmp("-d", argv[0])) {
        execlp("cat", "cat", CONFIG->drainfile, NULL);
        perror("cmd_drainfile: execlp");
    } else if (argc > 0 && 0 == strcmp("-e", argv[0])) {
        char *editor = getenv("EDITOR");
        if (!editor || !*editor) { editor = "vi"; }
        execlp(editor, editor, CONFIG->drainfile, NULL);
        perror("cmd_drainfile: execlp");
    } else {
        printf("%s\n", CONFIG->drainfile);
    }
    return 0;
}

typedef int(*CommandFunction)(int, char**);

typedef struct {
    char const * const name;
    const CommandFunction fn;
    char const * const short_help;
} Command;

const Command COMMANDS[] = {
    { "up",        cmd_up,        "up [NAME ...]                      -- start one, more or all processes" },
    { "status",    cmd_status,    "status                             -- status of drain server" },
    { "server",    cmd_server,    "server [NAME ...]                  -- start drain server" },
    { "restart",   cmd_restart,   "restart [NAME ...]                 -- restart one, more or all processes" },
    { "ping",      cmd_ping,      "ping                               -- ping drain server" },
    { "help",      cmd_help,      "help                               -- show this help" },
    { "halt",      cmd_halt,      "halt [NAME ...]                    -- stop one, more or all processes" },
    { "drainfile", cmd_drainfile, "drainfile [-d|-e]                  -- show / edit drainfile" },
    { "attach",    cmd_attach,    "attach NAME ...                    -- retreive output of processes" },
    { "add",       cmd_add,       "add [-s] NAME COLOR CMD [ARGS ...] -- add a new process (-s will start it)" },
};

int cmd_help(int argc, char** argv) {
    (void)argc;
    (void)argv;
    puts("drain [options] [CMD]\n");
    puts("options");
    puts("  -f DRAINFILE    -- configure drainfile path");
    puts("  -S SOCKET-PATH  -- specify full socket path");
    puts("  -h              -- help");
    puts("  -k              -- keep server running");
    puts("  -v              -- verbose");
    puts("  -w              -- line wrapping in log output");
    puts("  -W              -- no line wrapping in log output\n");
    puts("commands");
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
