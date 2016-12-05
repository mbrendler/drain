#include "process_mock.h"
#include "../src/process_list.h"
#include <string.h>

static ProcessList *list = NULL;

ProcessList* process_list() {
    return list;
}

void process_list_set(ProcessList* l) {
    if (list) {
        process_list_free(list);
    }
    list = l;
}

struct ProcessCalls process_calls;

void init_process_calls() {
    process_calls = (struct ProcessCalls){0, 0, 0, 0, 0, 0, 0, 0};
}

void process_init(Process *p, const char *name, const char *cmd, int color, int fd) {
    process_calls.init++;
    p->name = (char*)name;
    p->cmd = (char*)cmd;
    p->color = color;
    p->fd = fd;
}

void process_clear(Process *p) {
    process_calls.free++;
    p->name = NULL;
    p->cmd = NULL;
    p->color = -1;
    p->fd = -1;
}

void process_start(Process *p) {
    process_calls.start++;
    p->fd = cfStartCalled;
}

int process_forward(Process *p) {
    process_calls.forward++;
    p->fd = cfForwardCalled;
    return 0;
}

void process_stop(Process *p) {
    process_calls.stop++;
    p->fd = cfStopCalled;
}

void process_add_output_fd(Process *p, int fd) {
    p->fd = fd;
    process_calls.add_output_fd++;
}

void process_remove_output_fd(Process *p, int index) {
    p->fd = index;
    process_calls.remove_output_fd++;
}

int process_serialize(Process *p, char* buffer, int buf_size) {
    strncpy(buffer, p->name, buf_size);
    return strlen(buffer) + 1;
}
