#pragma once

#include <stdio.h>
#include <unistd.h>

struct Process {
    int color;
    pid_t pid;
    int fd;
    FILE* f;
    char *name;
    char *cmd;
};

typedef struct Process Process;

void process_init(Process *p, const char *name, const char *cmd, int color);

void process_free(Process *p);

void process_start(Process *p);

int process_forward(const Process *p);

void process_stop(Process *p);

void process_kill(Process *p);
