#include "tests.h"
#include "../src/actions.h"
#include "../src/process_list.h"
#include "../src/helpers.h"

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

enum CallFds {
    cfNoneCalled,
    cfStartCalled,
    cfForwardCalled,
    cfStopCalled,
};

struct ProcessCalls {
    int init;
    int free;
    int start;
    int forward;
    int stop;
    int add_output_fd;
    int remove_output_fd;
    int serialize;
} process_calls;

void init_process_calls() {
    process_calls = (struct ProcessCalls){0, 0, 0, 0, 0, 0, 0, 0};
}

void process_init(Process *p, const char *name, const char *cmd, int color, int fd) {
    process_calls.init++;
    p->name = (char*)name;
    p->cmd = (char*)cmd;
    p->color = color;
    p->fd = fd;
}

void process_free(Process *p) {
    process_calls.free++;
    p->name = NULL;
    p->cmd = NULL;
    p->color = -1;
    p->fd = -1;
}

void process_start(Process *p) {
    process_calls.start++;
    p->fd = cfStartCalled;
}

int process_forward(Process *p) {
    process_calls.forward++;
    p->fd = cfForwardCalled;
    return 0;
}

void process_stop(Process *p) {
    process_calls.stop++;
    p->fd = cfStopCalled;
}

void process_add_output_fd(Process *p, int fd) {
    p->fd = fd;
    process_calls.add_output_fd++;
}

void process_remove_output_fd(Process *p, int index) {
    p->fd = index;
    process_calls.remove_output_fd++;
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

void test_action_up() {
    // start all:
    Message in = {.nr=mnUp, .size=0};
    Message out;
    ASSERT_INT(0, perform_action(42, &in, &out));
    ASSERT_INT(mnUp, out.nr);
    ASSERT_INT(0, out.size);
    ASSERT_INT(cfStartCalled, process_list()->p.fd);
    ASSERT_INT(cfStartCalled, process_list()->n->p.fd);

    // start only one
    process_list()->p.fd = cfNoneCalled;
    process_list()->n->p.fd = cfNoneCalled;
    in = (Message){.nr=mnUp, .size=3, .content="p2"};
    ASSERT_INT(0, perform_action(42, &in, &out));
    ASSERT_INT(mnUp, out.nr);
    ASSERT_INT(0, out.size);
    ASSERT_INT(cfNoneCalled, process_list()->p.fd);
    ASSERT_INT(cfStartCalled, process_list()->n->p.fd);
}

void test_action_stop() {
    // start all:
    Message in = {.nr=mnDown, .size=0};
    Message out;
    ASSERT_INT(0, perform_action(42, &in, &out));
    ASSERT_INT(mnDown, out.nr);
    ASSERT_INT(0, out.size);
    ASSERT_INT(cfStopCalled, process_list()->p.fd);
    ASSERT_INT(cfStopCalled, process_list()->n->p.fd);

    // stop only one
    process_list()->p.fd = cfNoneCalled;
    process_list()->n->p.fd = cfNoneCalled;
    in = (Message){.nr=mnDown, .size=3, .content="p2"};
    ASSERT_INT(0, perform_action(42, &in, &out));
    ASSERT_INT(mnDown, out.nr);
    ASSERT_INT(0, out.size);
    ASSERT_INT(cfNoneCalled, process_list()->p.fd);
    ASSERT_INT(cfStopCalled, process_list()->n->p.fd);
}

void test_action_restart() {
    // start all:
    Message in = {.nr=mnRestart, .size=0};
    Message out;
    ASSERT_INT(0, perform_action(42, &in, &out));
    ASSERT_INT(mnRestart, out.nr);
    ASSERT_INT(0, out.size);
    ASSERT_INT(cfStartCalled, process_list()->p.fd);
    ASSERT_INT(cfStartCalled, process_list()->n->p.fd);
    ASSERT_INT(2, process_calls.stop);
    ASSERT_INT(2, process_calls.start);

    // restart only one
    process_list()->p.fd = cfNoneCalled;
    process_list()->n->p.fd = cfNoneCalled;
    in = (Message){.nr=mnRestart, .size=3, .content="p2"};
    ASSERT_INT(0, perform_action(42, &in, &out));
    ASSERT_INT(mnRestart, out.nr);
    ASSERT_INT(0, out.size);
    ASSERT_INT(cfNoneCalled, process_list()->p.fd);
    ASSERT_INT(cfStartCalled, process_list()->n->p.fd);
}

// TODO:
// void test_action_log() {
// }

void test_action_add() {
    Message in = {.nr=mnAdd};
    in.size = serialize_string_array(
        (char*[]){"p3", "10", "cmd3"}, 3,
        in.content, sizeof(in.content)
    );
    Message out;

    ASSERT_INT(0, perform_action(42, &in, &out));
    ASSERT_INT(mnAdd, out.nr);
    ASSERT_INT(0, out.size);
    ProcessList *new = process_list()->n->n;
    ASSERT(new);
    ASSERT_STRING("p3", new->p.name);
    ASSERT_STRING("cmd3", new->p.cmd);
    ASSERT_INT(10, new->p.color);

    // p3 does already exist
    ASSERT_INT(0, perform_action(42, &in, &out));
    ASSERT_INT(-1, out.nr);
    ASSERT_STRING("Could not create process 'p3'.", out.content);
    ASSERT_INT((int)strlen(out.content) + 1, out.size);
}

void test_action_unknown() {
    Message in = {.nr=394};
    Message out;
    ASSERT_INT(-1, perform_action(42, &in, &out));
}

int main() {
    ProcessList *pl2 = process_list_new("p2", "cmd2", 3, cfNoneCalled);
    list = process_list_append(process_list_new("p1", "cmd1", 1, cfNoneCalled), &pl2);

    init_process_calls();
    test_action_ping();
    init_process_calls();
    test_action_status();
    init_process_calls();
    test_action_up();
    init_process_calls();
    test_action_stop();
    init_process_calls();
    test_action_restart();
    init_process_calls();
    /* test_action_log() */
    init_process_calls();
    test_action_add();
    init_process_calls();
    test_action_unknown();
}
