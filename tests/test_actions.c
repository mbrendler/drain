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

void process_init(Process *p, const char *name, const char *cmd, int color, int fd) {
    p->name = (char*)name;
    p->cmd = (char*)cmd;
    p->color = color;
    p->fd = fd;
}

void process_free(Process *p) {
    p->name = NULL;
    p->cmd = NULL;
    p->color = -1;
    p->fd = -1;
}

void process_start(Process *p) {
    p->fd = cfStartCalled;
}

int process_forward(Process *p) {
    p->fd = cfForwardCalled;
    return 0;
}

void process_stop(Process *p) {
    p->fd = cfStopCalled;
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

    // start only one
    process_list()->p.fd = cfNoneCalled;
    process_list()->n->p.fd = cfNoneCalled;
    in = (Message){.nr=mnDown, .size=3, .content="p2"};
    ASSERT_INT(0, perform_action(42, &in, &out));
    ASSERT_INT(mnDown, out.nr);
    ASSERT_INT(0, out.size);
    ASSERT_INT(cfNoneCalled, process_list()->p.fd);
    ASSERT_INT(cfStopCalled, process_list()->n->p.fd);
}

// TODO:
// void test_action_restart() {
// }

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

int main() {
    ProcessList *pl2 = process_list_new("p2", "cmd2", 3, cfNoneCalled);
    list = process_list_append(process_list_new("p1", "cmd1", 1, cfNoneCalled), &pl2);
    test_action_ping();
    test_action_status();
    test_action_up();
    test_action_stop();
    /* test_action_restart(); */
    /* test_action_log() */
    test_action_add();
}
