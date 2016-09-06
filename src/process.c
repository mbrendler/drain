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

void process_init(Process *p, const char *name, const char *cmd, int color, int fd) {
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
    p->out_fd_count = 0;
    p->out_fds = NULL;
    p->pid = -1;
    p->fd = fd;
    p->f = -1 == fd ? NULL : fdopen(p->fd, "r");
}

void process_start(Process *p) {
    if (p->pid >= 0 || p->fd > -1) {
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
    if (p->pid > 0) { waitpid(p->pid, NULL, 0); }
    printf("process stopped: %s\n", p->name);
    p->f = NULL;
    p->fd = -1;
    p->pid = -1;
}

void process_free(Process *p) {
    process_stop(p);
    while (p->out_fd_count > 0) {
        --p->out_fd_count;
        close(*p->out_fds);
    }
    free(p->out_fds);
    p->out_fds = NULL;
    free(p->name);
    p->name = NULL;
    free(p->cmd);
    p->cmd = NULL;
}

void process_add_output_fd(Process *p, int fd) {
    p->out_fds = realloc(p->out_fds, (p->out_fd_count + 1) * sizeof(fd));
    *(p->out_fds + p->out_fd_count) = fd;
    p->out_fd_count++;
}

void process_remove_output_fd(Process *p, int index) {
    close(p->out_fds[index]);
    if (1 == p->out_fd_count) {
        free(p->out_fds);
        p->out_fds = NULL;
        p->out_fd_count = 0;
    } else {
        p->out_fds[index] = p->out_fds[p->out_fd_count - 1];
        p->out_fds = realloc(p->out_fds, (p->out_fd_count - 1) * sizeof(int));
        p->out_fd_count--;
    }
}

void print_line(const char *name, const char *content, char sep, int color, int width) {
    fprintf(stdout, "\033[3%dm%s%c \033[39;49m", color, name, sep);
    fwrite(content, sizeof(*content), width, stdout);
    fwrite("\n", sizeof(char), 1, stdout);
}

int process_forward(Process *p) {
    if (!p->f) { return -1; }
    const bool line_wrap = CONFIG->line_wrap;
    const int width = CONFIG->term_width - strlen(p->name) - 2;
    while (fgets(BUFFER, sizeof(BUFFER), p->f)) {
        int len = strlen(BUFFER);
        for (int i = 0; i < p->out_fd_count; ++i) {
            if (-1 == write(p->out_fds[i], BUFFER, len)) {
                perror("write out_fd");
                process_remove_output_fd(p, i);
            }
        }
        if ('\n' == BUFFER[len - 1]) { len--; }
        if (!line_wrap || len <= width) {
            print_line(p->name, BUFFER, ':', p->color, len);
        } else {
            print_line(p->name, BUFFER, ':', p->color, width);
            len -= width;
            const char *a = BUFFER + width;
            while (len > width) {
                print_line(p->name, a, '^', p->color, width);
                len -= width;
                a += width;
            }
            if (len > 0) {
                print_line(p->name, a, '^', p->color, len);
            }
        }
    }
    if (feof(p->f)) {
        process_stop(p);
        return -1;
    } else if (ferror(p->f) && EAGAIN != errno) {
        perror("read");
        process_stop(p);
        return -1;
    }
    return 0;
}

int process_print_status(const Process* p) {
    printf("\033[3%dm%8s\033[39;49m | %s | %5d | %2d | %s\n",
        p->color,
        p->name,
        p->f ? "running" : "stopped",
        p->pid,
        p->out_fd_count,
        p->cmd
    );
    return 0;
}

int process_serialize(Process *p, char* buffer, int buf_size) {
    memcpy(buffer, &p->pid, sizeof(p->pid));
    int size = sizeof(p->pid);
    memcpy(buffer + size, &p->color, sizeof(p->color));
    size += sizeof(p->color);
    memcpy(buffer + size, &p->fd, sizeof(p->fd));
    size += sizeof(p->fd);
    memcpy(buffer + size, &p->out_fd_count, sizeof(p->out_fd_count));
    size += sizeof(p->out_fd_count);
    strncpy(buffer + size, p->name, buf_size - size);
    size += strlen(p->name) + 1;
    strncpy(buffer + size, p->cmd, buf_size - size);
    size += strlen(p->cmd) + 1;
    return size;
}

int process_deserialize(char* buffer, Process* p) {
    const char *b = buffer;
    memcpy(&p->pid, buffer, sizeof(p->pid));
    buffer += sizeof(p->pid);
    memcpy(&p->color, buffer, sizeof(p->color));
    buffer += sizeof(p->color);
    memcpy(&p->fd, buffer, sizeof(p->fd));
    buffer += sizeof(p->fd);
    memcpy(&p->out_fd_count, buffer, sizeof(p->out_fd_count));
    buffer += sizeof(p->out_fd_count);
    p->name = buffer;
    p->cmd = buffer + strlen(p->name) + 1;
    return buffer - b + strlen(p->name) + 1 + strlen(p->cmd) + 1;
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
