#include "process_mock.h"
#include "../src/process_list.h"
#include <string.h>
#include <stdlib.h>

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

void process_init(
  Process *p,
  const char *name,
  const char *cmd,
  int color,
  int fd
) {
  process_calls.init++;
  p->name = strdup(name);
  p->cmd = strdup(cmd);
  p->color = color;
  p->fd = fd;
  p->out_fd_count = 0;
  p->out_fds = NULL;
}

void process_clear(Process *p) {
  process_calls.free++;
  if (p->name) { free(p->name); }
  p->name = NULL;
  if (p->cmd) { free(p->cmd); }
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

bool process_add_output_fd(Process *p, int fd) {
  p->fd = fd;
  process_calls.add_output_fd++;
  return true;
}

void process_remove_output_fd_at(Process *p, size_t index) {
  p->fd = (int)index;
  process_calls.remove_output_fd++;
}

size_t process_serialize(Process *p, char* buffer, size_t buf_size) {
  strncpy(buffer, p->name, buf_size);
  return strlen(buffer) + 1;
}
