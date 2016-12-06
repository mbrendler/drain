#pragma once

#include <stdio.h>
#include <unistd.h>

typedef struct {
    int color;
    pid_t pid;
    int fd;
    FILE* f;
    int out_fd_count;
    int *out_fds;
    char *name;
    char *cmd;
} Process;

void process_init(Process *p, const char *name, const char *cmd, int color, int fd);

void process_clear(Process *p);

void process_start(Process *p);

int process_forward(Process *p);

void process_stop(Process *p);

void process_add_output_fd(Process *p, int fd);

void process_remove_output_fd_at(Process *p, int index);

int process_print_status(const Process* p);

int process_serialize(Process *p, char* buffer, int buf_size);

int process_deserialize(char* buffer, Process* p);
