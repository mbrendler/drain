#pragma once

#include <sys/select.h>

typedef struct ProcessList ProcessList;

ProcessList* process_list_new(const char *name, const char *cmd, int color);

void process_list_process_start(ProcessList* l, int namesc, char **names);

ProcessList* process_list_free_element(ProcessList* l);

void process_list_free(ProcessList* l);

int process_list_max_fd(ProcessList *l, int fd);

int process_list_init_fd_set(ProcessList *l, fd_set* set);

ProcessList* process_list_forward(ProcessList *l, fd_set* set);

ProcessList *process_list_append(ProcessList *l, ProcessList *n);

int process_list_status(ProcessList* l, char* out, int out_size);
