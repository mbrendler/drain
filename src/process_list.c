#include "process_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

ProcessList* process_list_new(const char *name, const char *cmd, int color, int fd) {
    ProcessList *e = malloc(sizeof(ProcessList));
    if (!e) {
        perror("malloc e");
        exit(1);
    }
    e->n = NULL;
    process_init(&(e->p), name, cmd, color, fd);
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
        process_stop(&l->p);
    }
    process_list_process_kill(l->n, namesc, names);
}

void process_list_process_kill(ProcessList* l, int namesc, char **names) {
    if (!l) { return; }
    if (!namesc || 0 <= string_list_contains(l->p.name, namesc, names)) {
        process_kill(&l->p);
    }
    process_list_process_kill(l->n, namesc, names);
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
    for (int i = 0; i < l->p.out_fd_count; ++i) {
        fd = fd > l->p.out_fds[i] ? fd : l->p.out_fds[i];
    }
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
        for (int i = 0; i < l->p.out_fd_count; ++i) {
            FD_SET(l->p.out_fds[i], set);
        }
        l = l->n;
    }
    return result;
}

void process_list_forward(ProcessList *l, fd_set* set) {
    if (!l) { return; }
    if (l->p.fd >= 0) {
        if (FD_ISSET(l->p.fd, set)) {
            process_forward(&(l->p));
        } else {
            for (int i = 0; i < l->p.out_fd_count; ++i) {
                if (FD_ISSET(l->p.out_fds[i], set)) {
                    if (-1 == write(l->p.out_fds[i], NULL, 0)) {
                        process_remove_output_fd(&l->p, i);
                    }
                }
            }
        }
    }
    process_list_forward(l->n, set);
}

ProcessList *process_list_append(ProcessList *l, ProcessList **n) {
    if (l) {
        if (0 == strcmp(l->p.name, (*n)->p.name)) {
            free(*n);
            *n = NULL;
            fprintf(stderr, "Process '%s' does already exist.\n", l->p.name);
            return l;
        }
        l->n = process_list_append(l->n, n);
        return l;
    }
    return *n;
}

Process* process_list_add_ouput_fd(ProcessList *l, int fd, char *name) {
    if (!l) { return NULL; }
    if (0 == strcmp(name, l->p.name)) {
        process_add_output_fd(&l->p, fd);
        return &l->p;
    }
    return process_list_add_ouput_fd(l->n, fd, name);
}
