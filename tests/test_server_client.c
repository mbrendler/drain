#include "tests.h"
#include "../src/server.h"
#include "../src/client.h"
#include "../src/config.h"

// Mocks:

int cmd_help(int argc, char** argv) {
    (void)argc;
    (void)argv;
    return 0;
}

static struct {
    Message in;
    Message out;
} perform_action_mock;

int perform_action(int fd, Message* in, Message* out) {
    (void)fd;
    perform_action_mock.in = *in;
    *out = perform_action_mock.out;
    return 0;
}

// End Mocks

static Server s;

void cleanup() {
    server_stop(&s);
}

void test_client_server() {
    fd_set set;
    memset(&set, 0xff, sizeof(set));

    server_init(&s);
    ASSERT_INT(-1, s.fd);

    ASSERT_INT(0, server_start(&s));
    Message out = {.nr=23, .size=12, .content="hello world"};

    perform_action_mock.out = (Message){.nr=394, .size=10, .content="a content"};

    Client c;
    client_init(&c);
    ASSERT_INT(-1, c.fd);
    ASSERT_INT(0, client_start(&c))
    ASSERT_INT(0, client_send(&c, &out));
    ASSERT_INT(0, server_incomming(&s, &set));
    ASSERT_INT(23, perform_action_mock.in.nr);
    ASSERT_INT(12, perform_action_mock.in.size);
    ASSERT_STRING("hello world", perform_action_mock.in.content);

    Message in;
    ASSERT_INT(0, client_receive(&c, &in));
    ASSERT_INT(394, in.nr);
    ASSERT_INT(10, in.size);
    ASSERT_STRING("a content", in.content);

    client_stop(&c);
    ASSERT_INT(-1, c.fd);
    server_stop(&s);
    ASSERT_INT(-1, s.fd);
}

int main() {
    server_init(&s);
    atexit(cleanup);

    config_parse_args(3, (char*[]){"program-name", "-S", "/tmp/_test_socket"});

    test_client_server();
}
