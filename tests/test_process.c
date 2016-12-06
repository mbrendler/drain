#include "tests.h"
#include "../src/process.h"

void test_init() {
    Process p;
    process_init(&p, "a-process-name", "a-process-command", 23, 42);

    ASSERT_INT(23, p.color);
    ASSERT_INT(-1, p.pid);
    ASSERT_INT(42, p.fd);
    ASSERT(NULL == p.f);
    ASSERT_INT(0, p.out_fd_count);
    ASSERT(NULL == p.out_fds);
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
    ASSERT(NULL == p.f);
    ASSERT_INT(0, p.out_fd_count);
    ASSERT(NULL == p.out_fds);
    ASSERT(NULL == p.name);
    ASSERT(NULL == p.cmd);
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

void test_remove_output_fd() {
    Process p;
    process_init(&p, "a-process-name", "a-process-command", 23, 42);
    process_add_output_fd(&p, 394);
    process_add_output_fd(&p, 123);
    process_add_output_fd(&p, 456);
    process_add_output_fd(&p, 789);

    process_remove_output_fd_at(&p, 4); // index too high
    ASSERT_INT(4, p.out_fd_count);
    process_remove_output_fd_at(&p, -1); // index too low
    ASSERT_INT(4, p.out_fd_count);

    process_remove_output_fd_at(&p, 1);
    ASSERT_INT(3, p.out_fd_count);
    ASSERT_INT(394, *p.out_fds);
    ASSERT_INT(789, *(p.out_fds + 1));
    ASSERT_INT(456, *(p.out_fds + 2));

    process_remove_output_fd_at(&p, 0);
    ASSERT_INT(2, p.out_fd_count);
    ASSERT_INT(456, *p.out_fds);
    ASSERT_INT(789, *(p.out_fds + 1));

    process_remove_output_fd_at(&p, 1);
    ASSERT_INT(1, p.out_fd_count);
    ASSERT_INT(456, *p.out_fds);

    process_remove_output_fd_at(&p, 0);
    ASSERT_INT(0, p.out_fd_count);
    ASSERT(NULL == p.out_fds);
}

// int process_print_status(const Process* p);

void test_process_serialize_deserialize() {
    char buffer[1024];
    Process p;
    process_init(&p, "a-process-name", "a-process-command", 23, 42);

    int serialize_size = process_serialize(&p, buffer, sizeof(buffer));
    ASSERT_INT(49, serialize_size);
    ASSERT_BYTES(49,
        "\xff\xff\xff\xff"     // pid
        "\x17\0\0\0"           // color
        "*\0\0\0"              // fd
        "\0\0\0\0"             // out-fd-count
        "a-process-name\0"     // name
        "a-process-command\0", // command
        buffer
    );

    int deserialize_size =  process_deserialize(buffer, &p);
    ASSERT_INT(49, deserialize_size);

    ASSERT_INT(-1, p.pid);
    ASSERT_INT(23, p.color);
    ASSERT_INT(42, p.fd);
    ASSERT_INT(0, p.out_fd_count);
    ASSERT_STRING("a-process-name", p.name);
    ASSERT_STRING("a-process-command", p.cmd);
}

int main() {
    test_init();
    test_clear();
    test_add_output_fd();
    test_remove_output_fd();
    test_process_serialize_deserialize();
}
