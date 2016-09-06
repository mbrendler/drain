#pragma once

#include "process.h"
#include <sys/select.h>

typedef struct ProcessList ProcessList;

struct ProcessList {
    ProcessList *n;
    Process p;
};

typedef struct Process Process;

ProcessList* process_list_new(const char *name, const char *cmd, int color, int fd);

void process_list_process_start(ProcessList* l, int namesc, char **names);

void process_list_process_stop(ProcessList* l, int namesc, char **names);

void process_list_process_kill(ProcessList* l, int namesc, char **names);

ProcessList* process_list_free_element(ProcessList* l);

void process_list_free(ProcessList* l);

int process_list_max_fd(ProcessList *l, int fd);

int process_list_init_fd_set(ProcessList *l, fd_set* set);

void process_list_forward(ProcessList *l, fd_set* set);

ProcessList *process_list_append(ProcessList *l, ProcessList **n);

Process* process_list_add_ouput_fd(ProcessList *l, int fd, char *name);

#define process_list_each(l, fn, ...) {                          \
    ProcessList *lIsT = l;                                       \
    while (NULL != lIsT && 0 == fn(&lIsT->p, ## __VA_ARGS__ )) { \
        lIsT = lIsT->n;                                          \
    }                                                            \
}
