#include "tests.h"
#include "../src/actions.h"
#include "../src/process_list.h"

// Mocks:

int cmd_help(int argc, char** argv) {
    (void)argc;
    (void)argv;
    return 0;
}

ProcessList *list = NULL;

ProcessList* process_list() {
    return list;
}

void process_init(Process *p, const char *name, const char *cmd, int color, int fd) {
    p->name = strdup(name);
    p->cmd = strdup(cmd);
    p->color = color;
    p->fd = fd;
}

void process_free(Process *p) {
    free(p->name);
    p->name = NULL;
    free(p->cmd);
    p->cmd = NULL;
    p->color = -1;
    p->fd = -1;
}

void process_start(Process *p) {
    p->fd = 0xff;
}

int process_forward(Process *p) {
    p->fd = 0xfe;
    return 0;
}

void process_stop(Process *p) {
    p->fd = 0xfd;
}

void process_add_output_fd(Process *p, int fd) {
    p->fd = fd;
}

void process_remove_output_fd(Process *p, int index) {
    p->fd = index;
}

int process_serialize(Process *p, char* buffer, int buf_size) {
    strncpy(buffer, p->name, buf_size);
    return strlen(buffer) + 1;
}

// End Mocks

void test_action_ping() {
    Message in = {.nr=mnPing, .size=12, .content="hello world"};
    Message out;
    ASSERT_INT(0, perform_action(23, &in, &out));
    ASSERT_INT(mnPing, out.nr);
    ASSERT_INT(12, out.size);
    ASSERT_STRING("hello world", out.content);
}

void test_action_status() {
    Message in = {.nr=mnStatus, .size=0};
    Message out;
    ASSERT_INT(0, perform_action(42, &in, &out));
    ASSERT_INT(mnStatus, out.nr);
    ASSERT_INT(6, out.size);
    ASSERT_BYTES(6, "p1\0p2\0", out.content);
}

int main() {
    ProcessList *pl2 = process_list_new("p2", "cmd2", 3, 4);
    list = process_list_append(process_list_new("p1", "cmd1", 1, 2), &pl2);
    test_action_ping();
    test_action_status();
}
