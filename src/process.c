#include "process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static void close_all_fds_from(int fd);
static int process_open(const char *cmd, int *fd);

char BUFFER[4096];

void process_init(Process *p, const char *name, const char *cmd, int color) {
    p->color = color;
    const int name_len = strlen(name) + 1;
    p->name = malloc(name_len);
    if (!p->name) {
        perror("malloc p->name");
        exit(1);
    }
    memcpy(p->name, name, name_len);
    const int cmd_len = strlen(cmd) + 1;
    p->cmd = malloc(cmd_len);
    if (!p->cmd) {
        perror("malloc p->cmd");
        exit(1);
    }
    memcpy(p->cmd, cmd, cmd_len);
    p->pid = -1;
    p->fd = -1;
    p->f = NULL;
}

void process_start(Process *p) {
    if (p->pid >= 0) {
        fprintf(stderr, "process already started: %s\n", p->name);
        return;
    }
    p->pid = process_open(p->cmd, &(p->fd));
    if (p->pid < 0) {
        perror("process_open");
    } else {
        p->f = fdopen(p->fd, "r");
    }
}

int process_forward(const Process *p) {
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

void process_stop(Process *p) {
    if (!p->f) { return; }
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

// private --

static void close_all_fds_from(int fd) {
    const int maxfd = sysconf(_SC_OPEN_MAX);
    while(fd <= maxfd) {
        close(fd++);
    }
}

static int process_open(const char *cmd, int *fd) {
    int pipeline[2];
    if (-1 == pipe(pipeline)) { return -1; }
    const pid_t pid = fork();
    if (-1 == pid) {
        return -1;
    } else if (0 == pid) {
        if (dup2(pipeline[1], STDOUT_FILENO) == -1) {
            perror("dup2 stdout");
            exit(EXIT_FAILURE);
        }
        if (dup2(pipeline[1], STDERR_FILENO) == -1) {
            perror("dup2 stderr");
            exit(EXIT_FAILURE);
        }
        // TODO: close STDIN_FILENO
        close_all_fds_from(3);
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
