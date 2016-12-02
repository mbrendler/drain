#include "actions.h"
#include "helpers.h"
#include "process_list.h"
#include "process.h"
#include "cmd_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int action_ping(int fd, Message* in, Message* out) {
    (void)fd;
    *out = *in;
    return 0;
}

typedef struct {
    size_t pos;
    size_t size;
    char* b;
} Buffer;

int serialize_processes(Process *p, Buffer* b) {
    b->pos += process_serialize(p, b->b + b->pos, b->size - b->pos);
    return 0;
}

int action_status(int fd, Message* in, Message* out) {
    (void)fd;
    Buffer b = {.pos=0, .size=sizeof(out->content), .b=out->content};
    process_list_each(process_list(), serialize_processes, &b);
    out->size = b.pos;
    out->nr = in->nr;
    return 0;
}

int action_up(int fd, Message* in, Message* out) {
    (void)fd;
    char **names = NULL;
    const int count = deserialize_string_array(in->content, in->size, &names);
    process_list_process_start(process_list(), count, names);
    free(names);
    out->nr = in->nr;
    out->size = 0;
    return 0;
}

int action_down(int fd, Message* in, Message* out) {
    (void)fd;
    char **names = NULL;
    const int count = deserialize_string_array(in->content, in->size, &names);
    process_list_process_kill(process_list(), count, names);
    free(names);
    out->nr = in->nr;
    out->size = 0;
    return 0;
}

int action_restart(int fd, Message* in, Message* out) {
    (void)fd;
    char **names = NULL;
    const int count = deserialize_string_array(in->content, in->size, &names);
    ProcessList *l = process_list();
    process_list_process_stop(l, count, names);
    process_list_process_start(l, count, names);
    free(names);
    out->nr = in->nr;
    out->size = 0;
    return 0;
}

int action_log(int fd, Message* in, Message* out) {
    Process* p = process_list_add_ouput_fd(process_list(), fd, in->content);
    if (NULL == p) {
        out->nr = -2;
        out->size = 0;
        return 0;
    } else {
        out->nr = 0;
        out->size = process_serialize(p, out->content, sizeof(out->content));
    }
    return 1;
}

int action_add(int fd, Message* in, Message* out) {
    (void)fd;
    char **args;
    const int count = deserialize_string_array(in->content, in->size, &args);
    if (count != 3) { return -1; }
    printf("%s : %s : %s\n", args[0], args[1], args[2]);
    ProcessList* new = process_list_new(args[0], args[2], atoi(args[1]), -1);
    process_list_append(process_list(), &new);
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

typedef int(*ActionFunctionServer)(int fd, Message* in, Message* out);

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
    [mnLog]     = { "log",     action_log },
};

int perform_action(int fd, Message* in, Message* out) {
    if (in->nr < (int16_t)(sizeof(ACTIONS) / sizeof(*ACTIONS))) {
        printf("call action: %s (%d)\n", ACTIONS[in->nr].name, in->nr);
        return ACTIONS[in->nr].fn(fd, in, out);
    }
    return -1;
}
