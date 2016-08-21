#pragma once

#include <stdio.h>
#include <unistd.h>

struct Process {
    int color;
    pid_t pid;
    int fd;
    FILE* f;
    char *name;
};

typedef struct Process Process;

void process_init(Process *p, const char *name, const char *cmd, int color);

int process_forward(const Process *p);

void process_destroy(Process *p);
