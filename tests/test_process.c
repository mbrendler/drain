#include "tests.h"
#include "../src/process.h"

void test_init() {
    Process p;
    process_init(&p, "a-process-name", "a-process-command", 23, 42);

    ASSERT_INT(23, p.color);
    ASSERT_INT(-1, p.pid);
    ASSERT_INT(42, p.fd);
    ASSERT(!p.f);
    ASSERT_INT(0, p.out_fd_count);
    ASSERT(!p.out_fds);
    ASSERT_STRING("a-process-name", p.name);
    ASSERT_STRING("a-process-command", p.cmd);
}

void test_clear() {
    Process p;
    process_init(&p, "a-process-name", "a-process-command", 23, 42);
    process_clear(&p);

    ASSERT_INT(23, p.color);
    ASSERT_INT(-1, p.pid);
    ASSERT_INT(42, p.fd);
    ASSERT(!p.f);
    ASSERT_INT(0, p.out_fd_count);
    ASSERT(!p.out_fds);
    ASSERT(!p.name);
    ASSERT(!p.cmd);
}

// void process_start(Process *p);
// int process_forward(Process *p);
// void process_stop(Process *p);

void test_add_output_fd() {
    Process p;
    process_init(&p, "a-process-name", "a-process-command", 23, 42);

    process_add_output_fd(&p, 394);
    ASSERT_INT(1, p.out_fd_count);
    ASSERT_INT(394, *p.out_fds);

    process_add_output_fd(&p, 123);
    ASSERT_INT(2, p.out_fd_count);
    ASSERT_INT(394, *p.out_fds);
    ASSERT_INT(123, *(p.out_fds + 1));
}

// void process_remove_output_fd(Process *p, int index);
// int process_print_status(const Process* p);
// int process_serialize(Process *p, char* buffer, int buf_size);
// int process_deserialize(char* buffer, Process* p);

int main() {
    test_init();
    test_clear();
    test_add_output_fd();
}
