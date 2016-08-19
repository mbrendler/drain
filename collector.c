#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>

int co_popen(char *cmd[], int *fd) {
    int pipeline[2];
    if (-1 == pipe(pipeline)) { return -1; }
    const pid_t pid = fork();
    if (-1 == pid) {
        return -1;
    } else if (0 == pid) {
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
        if (-1 == execvp(cmd[0], cmd)) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }
    close(pipeline[1]);
    *fd = pipeline[0];

    fcntl(*fd, F_SETFL, fcntl(*fd, F_GETFL, 0)| O_NONBLOCK);

    return pid;
}

typedef struct {
    pid_t pid;
    int fd;
    size_t name_len;
    const char *name;
} CoProcess;

char BUFFER[4096];

ssize_t co_forward(CoProcess *p) {
    bool write_prefix = true;
    char *runner = "\n";
    for (;;) {
        const ssize_t size = read(p->fd, BUFFER, sizeof(BUFFER));
        if (size == 0) { return -1; }
        else if (size < 0) {
            if (EAGAIN != errno) {
                perror("read");
                return -1;
            }
            if (*(runner - 1) != '\n') {
                write(STDOUT_FILENO, "\n", 1);
            }
            return 0;
        }
        runner = BUFFER;
        char const * const end = BUFFER + size;
        for (; runner != end; ++runner) {
            if (write_prefix) {
                write(STDOUT_FILENO, p->name, p->name_len);
                write(STDOUT_FILENO, ": ", 2);
                write_prefix = false;
            }
            write(STDOUT_FILENO, runner, 1);
            if ('\n' == *runner) {
                write_prefix = true;
            }
        }
    }
    return 0;
}

void co_process_init(CoProcess *p, char *name, char *cmd[]) {
    p->name = name;
    p->name_len = strlen(name);
    p->pid = co_popen(cmd, &(p->fd));
    if (p->pid < 0) {
        perror("co_popen");
    }
}

void co_process_destroy(CoProcess *p) {
    close(p->fd);
    // TODO: interpret status:
    waitpid(p->pid, NULL, 0);
    printf("process stopped: %s\n", p->name);
}

struct CoProcessList {
    CoProcess p;
    struct CoProcessList *n;
};

struct CoProcessList* co_process_list_new(char *name, char *cmd[]) {
    struct CoProcessList *e = malloc(sizeof(CoProcess));
    co_process_init(&(e->p), name, cmd);
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
        if (co_forward(&(l->p)) < 0) {
            l = co_process_list_free_element(l);
        }
    }
    if (l) { l->n = co_process_list_forward(l->n, set); }
    return l;
}

int main() {
    char *cmd1[] = {"tail", "-f", "1.txt", "2.txt", NULL};
    char *cmd2[] = {"ls", NULL};
    struct CoProcessList *l = co_process_list_new("tail", cmd1);
    l->n = co_process_list_new("ls", cmd2);

    fd_set set;
    const int max = co_process_list_max_fd(l, -1);
    while (l) {
        FD_ZERO(&set);
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
