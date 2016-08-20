#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>

int co_popen(const char *cmd, const char *wd, int *fd) {
    int pipeline[2];
    if (-1 == pipe(pipeline)) { return -1; }
    const pid_t pid = fork();
    if (-1 == pid) {
        return -1;
    } else if (0 == pid) {
        if (wd && (chdir(wd) == -1)) {
            perror("chdir");
            exit(EXIT_FAILURE);
        }
        close(pipeline[0]);
        if (dup2(pipeline[1], STDOUT_FILENO) == -1) {
            perror("dup2 stdout");
            exit(EXIT_FAILURE);
        }
        if (dup2(pipeline[1], STDERR_FILENO) == -1) {
            perror("dup2 stderr");
            exit(EXIT_FAILURE);
        }
        close(pipeline[1]);
        if (-1 == execlp("sh", "sh", "-c", cmd, NULL)) {
            perror("execlp");
            exit(EXIT_FAILURE);
        }
    }
    close(pipeline[1]);
    *fd = pipeline[0];

    fcntl(*fd, F_SETFL, fcntl(*fd, F_GETFL, 0) | O_NONBLOCK);

    return pid;
}

typedef struct {
    int color;
    pid_t pid;
    int fd;
    FILE* f;
    char *name;
} CoProcess;

char BUFFER[4096];

ssize_t co_process_forward(const CoProcess *p) {
    while (fgets(BUFFER, sizeof(BUFFER), p->f)) {
        fprintf(stdout, "\033[3%dm%s: \033[39;49m%s", p->color, p->name, BUFFER);
    }
    if (feof(p->f)) {
        return -1;
    } else if (ferror(p->f) && EAGAIN != errno) {
        perror("read");
        return -1;
    }
    return 0;
}

void co_process_init(CoProcess *p, const char *name, const char *cmd, const char *wd, int color) {
    p->color = color;
    int name_len = strlen(name) + 1;
    p->name = malloc(name_len);
    if (!p->name) {
        perror("malloc p->name");
        exit(1);
    }
    memcpy(p->name, name, name_len);
    p->pid = co_popen(cmd, wd, &(p->fd));
    p->f = fdopen(p->fd, "r");
    if (p->pid < 0) {
        perror("co_popen");
    }
}

void co_process_destroy(CoProcess *p) {
    fclose(p->f);
    // TODO: interpret status:
    waitpid(p->pid, NULL, 0);
    printf("process stopped: %s\n", p->name);
    free(p->name);
    p->f = NULL;
    p->fd = -1;
    p->pid = -1;
    p->name = NULL;
}

struct CoProcessList {
    CoProcess p;
    struct CoProcessList *n;
};

struct CoProcessList* co_process_list_new(const char *name, const char *cmd, const char *wd, int color) {
    struct CoProcessList *e = malloc(sizeof(struct CoProcessList));
    if (!e) {
        perror("malloc e");
        exit(1);
    }
    co_process_init(&(e->p), name, cmd, wd, color);
    e->n = NULL;
    return e;
}

struct CoProcessList* co_process_list_free_element(struct CoProcessList* l) {
    struct CoProcessList *result = l->n;
    co_process_destroy(&(l->p));
    free(l);
    return result;
}

void co_process_list_free(struct CoProcessList* l) {
    if (!l) { return; }
    co_process_list_free(l->n);
    co_process_list_free_element(l);
}

int co_process_list_max_fd(struct CoProcessList *l, int fd) {
    fd = fd > l->p.fd ? fd : l->p.fd;
    return l->n ? co_process_list_max_fd(l->n, fd) : fd;
}

void co_process_list_init_fd_set(struct CoProcessList *l, fd_set* set) {
    FD_ZERO(set);
    struct CoProcessList *r = l;
    while (r) {
        FD_SET(r->p.fd, set);
        r = r->n;
    }
}

struct CoProcessList* co_process_list_forward(struct CoProcessList *l, fd_set* set) {
    if (!l) { return NULL; }
    if (FD_ISSET(l->p.fd, set) ) {
        if (co_process_forward(&(l->p)) < 0) {
            l = co_process_list_free_element(l);
        }
    }
    if (l) { l->n = co_process_list_forward(l->n, set); }
    return l;
}

struct CoProcessList *co_process_list_append(struct CoProcessList *l, struct CoProcessList *n) {
    if (l) {
        l->n = co_process_list_append(l->n, n);
        return l;
    }
    return n;
}

struct CoProcessList* co_read_config(const char* filename) {
    // tail:3:.:tail -f 1.txt 2.txt
    FILE* f = fopen(filename, "r");
    char *poss[3];
    struct CoProcessList *l = NULL;
    while (fgets(BUFFER, sizeof(BUFFER), f)) {
        char *r = BUFFER;
        int i = 0;
        while (*r) {
            if (':' == *r && i < 3) {
                *r = '\0';
                poss[i++] = r + 1;
            }
            ++r;
        }
        if (*(r - 1) == '\n') { *(r - 1) = '\0'; }
        struct CoProcessList* n = co_process_list_new(BUFFER, poss[2], poss[1], atoi(poss[0]));
        l = co_process_list_append(
            l, n
        );
        printf("%s : %s : %s : %s\n", BUFFER, *poss, poss[1], poss[2]);
    }
    if (ferror(f)) {
        fprintf(stderr, "could not load config file: %s", filename);
        exit(1);
    }
    return l;
}

int main() {
    struct CoProcessList *l = co_read_config("colfile");

    fd_set set;
    const int max = co_process_list_max_fd(l, -1);
    while (l) {
        co_process_list_init_fd_set(l, &set);
        if (-1 == select(max + 1, &set, NULL, NULL, NULL) && EINTR != errno) {
            perror("select");
            return EXIT_FAILURE;
        }
        l = co_process_list_forward(l, &set);
    }

    co_process_list_free(l);

    return EXIT_SUCCESS;
}
