#include "process_list.h"
#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

ProcessList* process_list_new(
    const char *name,
    const char *cmd,
    int color,
    int fd,
    char* groups
) {
    ProcessList *e = malloc(sizeof(ProcessList));
    if (!e) {
        perror("malloc e");
        return NULL;
    }
    e->n = NULL;
    process_init(&(e->p), name, cmd, color, fd);
    if (!e->p.name || !e->p.cmd) {
        free(e);
        return NULL;
    }
    e->groups = strdup(groups);
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

bool comma_separated_string_includes_word(const char* list, int wordsc, char** words) {
  for (--wordsc; wordsc >= 0; --wordsc) {
    const char* found = strstr(list, words[wordsc]);
    if (found) {
      if (found == list || *(found - 1) == ',') {
        const size_t len = strlen(words[wordsc]);
        if (*(found + len) == ',' || *(found + len) == 0) {
          return true;
        }
      }
    }
  }
  return false;
}

void process_list_process_start(ProcessList* l, int namesc, char **names) {
    if (!l) { return; }
    if (
      !namesc ||
      0 <= string_list_contains(l->p.name, namesc, names) ||
      comma_separated_string_includes_word(l->groups, namesc, names)
    ) {
        process_start(&(l->p));
    }
    process_list_process_start(l->n, namesc, names);
}

void process_list_process_stop(ProcessList* l, int namesc, char **names) {
    if (!l) { return; }
    if (!namesc || 0 <= string_list_contains(l->p.name, namesc, names)) {
        process_stop(&l->p);
    }
    process_list_process_stop(l->n, namesc, names);
}

static ProcessList* process_list_free_element(ProcessList* l) {
    ProcessList *result = l->n;
    process_clear(&(l->p));
    free(l->groups);
    free(l);
    return result;
}

void process_list_free(ProcessList* l) {
    if (!l) { return; }
    process_list_free(l->n);
    process_list_free_element(l);
}

int process_list_max_fd(ProcessList *l, int fd) {
    process_list_each(p, l, {
        fd = MAX(fd, p->fd);
        for (uint8_t i = 0; i < p->out_fd_count; ++i) {
            fd = MAX(fd, p->out_fds[i]);
        }
    });
    return fd;
}

int process_list_init_fd_set(ProcessList *l, fd_set* set) {
    FD_ZERO(set);
    int result = 0;
    process_list_each(p, l, {
        if (p->fd >= 0) {
            result++;
            FD_SET(p->fd, set);
        }
        for (uint8_t i = 0; i < p->out_fd_count; ++i) {
            FD_SET(p->out_fds[i], set);
        }
    });
    return result;
}

void process_list_forward(ProcessList *l, fd_set* set) {
    process_list_each(p, l, {
        if (p->fd >= 0) {
            if (FD_ISSET(p->fd, set)) {
                process_forward(p);
            } else {
                for (uint8_t i = 0; i < p->out_fd_count; ++i) {
                    if (FD_ISSET(p->out_fds[i], set)) {
                        if (-1 == write(p->out_fds[i], NULL, 0)) {
                            process_remove_output_fd_at(p, i);
                        }
                    }
                }
            }
        }
    });
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

Process* process_list_find_by_name(ProcessList *l, char *name) {
    process_list_each(p, l, {
        if (0 == strcmp(name, p->name)) {
            return p;
        }
    });
    return NULL;
}
