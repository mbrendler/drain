#include "process_list.h"
#include "process.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

struct ProcessList {
    ProcessList *n;
    Process p;
};

ProcessList* process_list_new(const char *name, const char *cmd, int color) {
    ProcessList *e = malloc(sizeof(ProcessList));
    if (!e) {
        perror("malloc e");
        exit(1);
    }
    e->n = NULL;
    process_init(&(e->p), name, cmd, color);
    return e;
}

int string_list_contains(const char* str, int strsc, char **strs) {
    for (--strsc; strsc >= 0; --strsc) {
        if (!strcmp(strs[strsc], str)) {
            return strsc;
        }
    }
    return -1;
}

void process_list_process_start(ProcessList* l, int namesc, char **names) {
    if (!l) { return; }
    if (!namesc || 0 <= string_list_contains(l->p.name, namesc, names)) {
        process_start(&(l->p));
    }
    process_list_process_start(l->n, namesc, names);
}

void process_list_process_stop(ProcessList* l, int namesc, char **names) {
    if (!l) { return; }
    if (!namesc || 0 <= string_list_contains(l->p.name, namesc, names)) {
        process_stop(&(l->p));
    }
    process_list_process_stop(l->n, namesc, names);
}

ProcessList* process_list_free_element(ProcessList* l) {
    ProcessList *result = l->n;
    process_free(&(l->p));
    free(l);
    return result;
}

void process_list_free(ProcessList* l) {
    if (!l) { return; }
    process_list_free(l->n);
    process_list_free_element(l);
}

int process_list_max_fd(ProcessList *l, int fd) {
    fd = fd > l->p.fd ? fd : l->p.fd;
    return l->n ? process_list_max_fd(l->n, fd) : fd;
}

int process_list_init_fd_set(ProcessList *l, fd_set* set) {
    FD_ZERO(set);
    int result = 0;
    while (l) {
        if (l->p.fd >= 0) {
            result++;
            FD_SET(l->p.fd, set);
        }
        l = l->n;
    }
    return result;
}

ProcessList* process_list_forward(ProcessList *l, fd_set* set) {
    if (!l) { return NULL; }
    if (l->p.fd >= 0 && FD_ISSET(l->p.fd, set)) {
        if (process_forward(&(l->p)) < 0) {
            process_stop(&l->p);
        }
    }
    if (l) { l->n = process_list_forward(l->n, set); }
    return l;
}

ProcessList *process_list_append(ProcessList *l, ProcessList *n) {
    if (l) {
        l->n = process_list_append(l->n, n);
        return l;
    }
    return n;
}

int process_list_status(ProcessList* l, char* out, int out_size) {
    int rest_size = out_size;
    while (l) {
        int size_written = snprintf(
            out, rest_size, "%8s | %s | %5d | %s\n",
            l->p.name, l->p.f ? "running" : "stopped", l->p.pid, l->p.cmd
        );
        out += size_written;
        rest_size -= size_written;
        l = l->n;
    }
    return out_size - rest_size;
}
