#include "actions.h"
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

int build_string_array(char* strs, int strs_size, char*** array) {
    // !!! FREE MEMORY 'array' !!!
    char *str = strs;
    int count = 0;
    while (str < strs + strs_size) {
        str += strlen(str) + 1;
        count++;
    }
    *array = calloc(count, sizeof(char*));
    str = strs;
    for (int i = 0; i < count; ++i) {
        (*array)[i] = str;
        str += strlen(str) + 1;
    }
    return count;
}

int action_up(Message* in, Message* out, ProcessList* l) {
    char **names = NULL;
    const int count = build_string_array(in->content, in->size, &names);
    process_list_process_start(l, count, names);
    free(names);
    out->nr = in->nr;
    out->size = 0;
    return 0;
}

int action_down(Message* in, Message* out, ProcessList* l) {
    char **names = NULL;
    const int count = build_string_array(in->content, in->size, &names);
    process_list_process_kill(l, count, names);
    free(names);
    out->nr = in->nr;
    out->size = 0;
    return 0;
}

int action_restart(Message* in, Message* out, ProcessList* l) {
    char **names = NULL;
    const int count = build_string_array(in->content, in->size, &names);
    process_list_process_stop(l, count, names);
    process_list_process_start(l, count, names);
    free(names);
    out->nr = in->nr;
    out->size = 0;
    return 0;
}

int action_add(Message* in, Message* out, ProcessList* l) {
    char **args;
    const int count = build_string_array(in->content, in->size, &args);
    if (count != 3) { return -1; }
    printf("%s : %s : %s\n", args[0], args[1], args[2]);
    process_list_append(l, process_list_new(args[0], args[2], atoi(args[1])));
    free(args);
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

int serialize_string_array(char** array, int size, char* buffer) {
    int buffer_size = 0;
    for (int i = 0; i < size; ++i) {
        int len = strlen(array[i]) + 1;
        memcpy(buffer, array[i], len);
        buffer += len;
        buffer_size += len;
    }
    return buffer_size;
}

int cmd_up(const char* name, int argc, char** argv) {
    (void)name;
    Message out, in;
    out.nr = mnUp;
    out.size = serialize_string_array(argv, argc, out.content);
    if (-1 == client_do(&out, &in)) { return -1; }
    return 0;
}

int cmd_down(const char* name, int argc, char** argv) {
    (void)name;
    Message out, in;
    out.nr = mnDown;
    out.size = serialize_string_array(argv, argc, out.content);
    if (-1 == client_do(&out, &in)) { return -1; }
    return 0;
}

int cmd_restart(const char* name, int argc, char** argv) {
    (void)name;
    Message out, in;
    out.nr = mnRestart;
    out.size = serialize_string_array(argv, argc, out.content);
    if (-1 == client_do(&out, &in)) { return -1; }
    return 0;
}

int cmd_add(const char* name, int argc, char** argv) {
    (void)name;
    if (argc < 3) { return -1; }
    Message out, in;
    out.nr = mnAdd;
    out.size = serialize_string_array(argv, argc, out.content);
    char *x = out.content + strlen(argv[0]) + 1 + strlen(argv[1]) + 1;
    while (x < out.content + out.size - 1) {
        if (*x == 0) { *x = ' '; }
        x++;
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
