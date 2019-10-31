#pragma once

#include "process.h"
#include <sys/select.h>

typedef struct ProcessList ProcessList;

struct ProcessList {
  ProcessList* n;
  char* groups;
  Process p;
};

ProcessList* process_list_new(
  const char *name,
  const char *cmd,
  int color,
  int fd,
  char* groups
);

void process_list_process_start(ProcessList* l, int namesc, char **names);

void process_list_process_stop(ProcessList* l, int namesc, char **names);

void process_list_free(ProcessList* l);

int process_list_max_fd(ProcessList *l, int fd);

int process_list_init_fd_set(ProcessList *l, fd_set* set);

void process_list_forward(ProcessList *l, fd_set* set);

ProcessList *process_list_append(ProcessList *l, ProcessList **n);

Process* process_list_find_by_name(ProcessList *l, char *name);

#define process_list_each(VAR_, LIST_, block) { \
  ProcessList *lIsT = LIST_;                    \
  while (NULL != lIsT) {                        \
    Process *VAR_ = &(lIsT->p);                 \
    block;                                      \
    lIsT = lIsT->n;                             \
  }                                             \
}
