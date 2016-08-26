#include "actions.h"
#include "helpers.h"
#include "process_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int action_ping(Message* in, Message* out, ProcessList* l) {
    (void)l;
    *out = *in;
    return 0;
}

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

int perform_action(Message* in, Message* out, ProcessList* l) {
    if (in->nr < (int16_t)(sizeof(ACTIONS) / sizeof(*ACTIONS))) {
        printf("call action: %s (%d)\n", ACTIONS[in->nr].name, in->nr);
        return ACTIONS[in->nr].fn(in, out, l);
    }
    return -1;
}
