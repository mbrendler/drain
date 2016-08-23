#include "actions.h"
#include <stdio.h>

enum MessageNumber {
    mnPing, mnStatus, mnUp
};

int action_ping(Message* in, Message* out, ProcessList* l) {
    (void)l;
    *out = *in;
    return 0;
}

#include "process_list.h"
#include <string.h>
#include <stdlib.h>

int action_status(Message* in, Message* out, ProcessList* l) {
    out->size = process_list_status(l, out->content, sizeof(out->content)) + 1;
    out->nr = in->nr;
    return 0;
}

int action_up (Message* in, Message* out, ProcessList* l) {
    char *name = in->content;
    int count = 0;
    while (name < in->content + in->size) {
        name += strlen(name) + 1;
        count++;
    }
    char **names = calloc(count, sizeof(char*));
    name = in->content;
    for (int i = 0; i < count; ++i) {
        names[i] = name;
        name += strlen(name) + 1;
    }
    process_list_process_start(l, count, names);
    free(names);
    out->nr = in->nr;
    out->size = 0;
    return 0;
}

typedef int(*ActionFunctionServer)(Message* in, Message* out, ProcessList* l);

typedef struct {
    const char* name;
    ActionFunctionServer fn;
} Action;

const Action ACTIONS[] = {
    [mnPing]   = { "ping",   action_ping },
    [mnStatus] = { "status", action_status },
    [mnUp]     = { "up",     action_up },
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

int cmd_ping(const char* name, int argc, char** argv) {
    (void)name;
    Message out, in;
    char *content = argc > 0 ? *argv : "hallo";
    memcpy(out.content, content, strlen(content) + 1);
    out.size = strlen(content) + 1;
    out.nr = mnPing;
    if (-1 == client_do(&out, &in)) { return -1; }
    puts(in.content);
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
    char *content = out.content;
    for (int i = 0; i < argc; ++i) {
        int len = strlen(argv[i]) + 1;
        memcpy(content, argv[i], len);
        content += len;
        out.size += len;
    }
    if (-1 == client_do(&out, &in)) { return -1; }
    return 0;
}

typedef int(*CommandFunction)(const char*, int, char**);

typedef struct {
    char const * const name;
    const CommandFunction fn;
} Command;

const Command COMMANDS[] = {
    { "ping",   cmd_ping },
    { "status", cmd_status },
    { "up",     cmd_up },
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
