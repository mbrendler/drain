#include "actions.h"
#include "helpers.h"
#include <stdio.h>

enum MessageNumber {
    mnPing, mnStatus, mnUp, mnDown, mnRestart, mnAdd,
};

int action_ping(Message* in, Message* out, ProcessList* l) {
    (void)l;
    *out = *in;
    return 0;
}

#include "process_list.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int action_status(Message* in, Message* out, ProcessList* l) {
    out->size = 1 + snprintf(
        out->content, sizeof(out->content), "pid: %d\n", getpid()
    );
    out->size += process_list_status(
        l, out->content + out->size - 1, sizeof(out->content) - out->size + 1
    );
    out->nr = in->nr;
    return 0;
}

int action_up(Message* in, Message* out, ProcessList* l) {
    char **names = NULL;
    const int count = deserialize_string_array(in->content, in->size, &names);
    process_list_process_start(l, count, names);
    free(names);
    out->nr = in->nr;
    out->size = 0;
    return 0;
}

int action_down(Message* in, Message* out, ProcessList* l) {
    char **names = NULL;
    const int count = deserialize_string_array(in->content, in->size, &names);
    process_list_process_kill(l, count, names);
    free(names);
    out->nr = in->nr;
    out->size = 0;
    return 0;
}

int action_restart(Message* in, Message* out, ProcessList* l) {
    char **names = NULL;
    const int count = deserialize_string_array(in->content, in->size, &names);
    process_list_process_stop(l, count, names);
    process_list_process_start(l, count, names);
    free(names);
    out->nr = in->nr;
    out->size = 0;
    return 0;
}

int action_add(Message* in, Message* out, ProcessList* l) {
    char **args;
    const int count = deserialize_string_array(in->content, in->size, &args);
    if (count != 3) { return -1; }
    printf("%s : %s : %s\n", args[0], args[1], args[2]);
    ProcessList* new = process_list_new(args[0], args[2], atoi(args[1]));
    process_list_append(l, &new);
    if (!new) {
        out->nr = -1;
        out->size = snprintf(out->content, sizeof(out->content),
            "Could not create process '%s'.", args[0]
        ) + 1;
    } else {
        out->nr = in->nr;
        out->size = 0;
    }
    free(args);
    return 0;
}

typedef int(*ActionFunctionServer)(Message* in, Message* out, ProcessList* l);

typedef struct {
    const char* name;
    ActionFunctionServer fn;
} Action;

const Action ACTIONS[] = {
    [mnPing]    = { "ping",    action_ping },
    [mnStatus]  = { "status",  action_status },
    [mnUp]      = { "up",      action_up },
    [mnDown]    = { "down",    action_down },
    [mnRestart] = { "restart", action_restart },
    [mnAdd]     = { "add",     action_add },
};

// restart, start, stop, log

int perform_action(Message* in, Message* out, ProcessList* l) {
    if (in->nr < (int16_t)(sizeof(ACTIONS) / sizeof(*ACTIONS))) {
        printf("call action: %s (%d)\n", ACTIONS[in->nr].name, in->nr);
        return ACTIONS[in->nr].fn(in, out, l);
    }
    return -1;
}

// TODO: split ----------------------------------------------------------------

#include "client.h"
#include <sys/time.h>

int cmd_ping(const char* name, int argc, char** argv) {
    (void)name;
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

int cmd_status(const char* name, int argc, char** argv) {
    (void)name;
    (void)argc;
    (void)argv;
    Message out = { mnStatus, 0, "" };
    Message in;
    if (-1 == client_do(&out, &in)) { return -1; }
    puts(in.content);
    return 0;
}

int cmd_up(const char* name, int argc, char** argv) {
    (void)name;
    Message out, in;
    out.nr = mnUp;
    out.size = serialize_string_array(argv, argc, out.content, sizeof(out.content));
    if (-1 == client_do(&out, &in)) { return -1; }
    return 0;
}

int cmd_down(const char* name, int argc, char** argv) {
    (void)name;
    Message out, in;
    out.nr = mnDown;
    out.size = serialize_string_array(argv, argc, out.content, sizeof(out.content));
    if (-1 == client_do(&out, &in)) { return -1; }
    return 0;
}

int cmd_restart(const char* name, int argc, char** argv) {
    (void)name;
    Message out, in;
    out.nr = mnRestart;
    out.size = serialize_string_array(argv, argc, out.content, sizeof(out.content));
    if (-1 == client_do(&out, &in)) { return -1; }
    return 0;
}

#include <stdbool.h>

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

int cmd_add(const char* name, int argc, char** argv) {
    (void)name;
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

typedef int(*CommandFunction)(const char*, int, char**);

typedef struct {
    char const * const name;
    const CommandFunction fn;
} Command;

const Command COMMANDS[] = {
    { "ping",    cmd_ping },
    { "status",  cmd_status },
    { "up",      cmd_up },
    { "down",    cmd_down },
    { "restart", cmd_restart },
    { "add",     cmd_add },
};

int perform_command(const char* name, int argc, char** argv) {
    const Command *cmd = COMMANDS + sizeof(COMMANDS) / sizeof(*COMMANDS);
    while (COMMANDS != cmd) {
        --cmd;
        if (!strcmp(name, cmd->name)) {
            return cmd->fn(name, argc, argv);
        }
    }
    printf("unknown command '%s'\n", name);
    return -1;
}
