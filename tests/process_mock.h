#pragma once

typedef struct ProcessList ProcessList;

extern struct ProcessCalls {
  int init;
  int free;
  int start;
  int forward;
  int stop;
  int add_output_fd;
  int remove_output_fd;
  int serialize;
} process_calls;

enum CallFds {
  cfNoneCalled,
  cfStartCalled,
  cfForwardCalled,
  cfStopCalled,
};

ProcessList* process_list(void);
void process_list_set(ProcessList* l);

void init_process_calls(void);
