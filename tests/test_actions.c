#include "tests.h"
#include "../src/actions.h"
#include "process_mock.h"
#include "../src/process_list.h"
#include "../src/helpers.h"

// Mocks:

int cmd_help(int argc, char** argv) {
    (void)argc;
    (void)argv;
    return 0;
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


void test_action_log() {
    Message in = {.nr=mnLog, .size=3, .content="p2"};
    Message out;

    ASSERT_INT(1, perform_action(42, &in, &out));
    ASSERT_INT(0, out.nr);
    ASSERT_INT(3, out.size);
    ASSERT_BYTES(3, "p2", out.content);
    Process *p = &(process_list()->n->p);
    ASSERT_INT(42, p->fd);

    // process not found
    in = (Message){.nr=mnLog, .size=8, .content="unknown"};
    ASSERT_INT(0, perform_action(394, &in, &out));
    ASSERT_INT(-2, out.nr);
    ASSERT_INT(0, out.size);
}

void test_action_add() {
    Message in = {.nr=mnAdd};
    in.size = (uint16_t)serialize_string_array(
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
    process_list_set(
        process_list_append(process_list_new("p1", "cmd1", 1, cfNoneCalled), &pl2)
    );

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
    test_action_log();
    init_process_calls();
    test_action_add();
    init_process_calls();
    test_action_unknown();

    process_list_set(NULL);
}
