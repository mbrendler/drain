#include "process.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

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
        printf("process started: %s\n", p->name);
    }
}

void process_kill(Process *p) {
    if (p->pid > 0) {
        kill(p->pid, SIGINT);
    }
}

void process_stop(Process *p) {
    if (!p->f) { return; }
    process_kill(p);
    fclose(p->f);
    // TODO: interpret status:
    waitpid(p->pid, NULL, 0);
    printf("process stopped: %s\n", p->name);
    p->f = NULL;
    p->fd = -1;
    p->pid = -1;
}

void process_free(Process *p) {
    process_stop(p);
    free(p->name);
    p->name = NULL;
    free(p->cmd);
    p->cmd = NULL;
}

int process_forward(const Process *p) {
    if (!p->f) { return -1; }
    const bool line_wrap = CONFIG->line_wrap;
    const int width = CONFIG->term_width - strlen(p->name) - 2;
    while (fgets(BUFFER, sizeof(BUFFER), p->f)) {
        char const * const last_line = BUFFER - width + strlen(BUFFER);
        char *a = BUFFER;
        if (line_wrap) {
          while (a < last_line) {
              char tmp = *(a + width);
              *(a + width) = '\0';
              fprintf(stdout, "\033[3%dm%s| \033[39;49m%s\n", p->color, p->name, a);
              a += width;
              *a = tmp;
          }
        }
        fprintf(stdout, "\033[3%dm%s: \033[39;49m%s", p->color, p->name, a);
    }
    if (feof(p->f)) {
        return -1;
    } else if (ferror(p->f) && EAGAIN != errno) {
        perror("read");
        return -1;
    }
    return 0;
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
