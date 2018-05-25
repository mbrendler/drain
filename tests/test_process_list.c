#include "tests.h"
#include "../src/process_list.h"
#include "process_mock.h"

void test_new() {
    ProcessList* l = process_list_new("name", "cmd", 23, 42, "");
    ASSERT(l);
    ASSERT_STRING("name", l->p.name);
    ASSERT_STRING("cmd", l->p.cmd);
    ASSERT_INT(23, l->p.color);
    ASSERT_INT(42, l->p.fd);
    ASSERT_INT(1, process_calls.init);
    process_list_free(l);
}

/* tested implicitly in test_actions.c */
/* void process_list_process_start(ProcessList* l, int namesc, char **names); */
/* void process_list_process_stop(ProcessList* l, int namesc, char **names); */

void test_max_fd() {
    ASSERT_INT(42, process_list_max_fd(process_list(), -1));
    ASSERT_INT(70, process_list_max_fd(process_list(), 70));
}

void test_forward() {
    fd_set set;
    FD_ZERO(&set);
    process_list_forward(process_list(), &set);
    ASSERT_INT(0, process_calls.forward);

    memset(&set, 0xff, sizeof(set));
    process_list_forward(process_list(), &set);
    ASSERT_INT(2, process_calls.forward);
}

void test_append() {
    ASSERT(process_list());
    ASSERT_STRING("p1", process_list()->p.name)
    ASSERT(process_list()->n);
    ASSERT_STRING("p2", process_list()->n->p.name)
}

void test_find() {
    Process* p = process_list_find_by_name(process_list(), "p2");
    ASSERT(p);
    ASSERT_STRING("p2", p->name);
    p = process_list_find_by_name(process_list(), "p1");
    ASSERT(p);
    ASSERT_STRING("p1", p->name);
    p = process_list_find_by_name(process_list(), "unknown");
    ASSERT(!p);
}

int main() {
    init_process_calls();
    test_new();

    ProcessList *pl2 = process_list_new("p2", "cmd2", 3, 23, "");
    process_list_set(
        process_list_append(process_list_new("p1", "cmd1", 1, 42, ""), &pl2)
    );

    init_process_calls();
    test_max_fd();
    init_process_calls();
    test_forward();
    init_process_calls();
    test_append();
    init_process_calls();
    test_find();
}
