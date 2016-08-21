#include "process_list.h"
#include "process.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

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

ProcessList* process_list_free_element(ProcessList* l) {
    ProcessList *result = l->n;
    process_destroy(&(l->p));
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

void process_list_init_fd_set(ProcessList *l, fd_set* set) {
    FD_ZERO(set);
    ProcessList *r = l;
    while (r) {
        FD_SET(r->p.fd, set);
        r = r->n;
    }
}

ProcessList* process_list_forward(ProcessList *l, fd_set* set) {
    if (!l) { return NULL; }
    if (FD_ISSET(l->p.fd, set) ) {
        if (process_forward(&(l->p)) < 0) {
            l = process_list_free_element(l);
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
