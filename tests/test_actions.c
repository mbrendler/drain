#include "tests.h"
#include "../src/actions.h"

// Mocks:

int cmd_help(int argc, char** argv) {
    (void)argc;
    (void)argv;
    return 0;
}

ProcessList* process_list() {
    return NULL;
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

int main() {
    test_action_ping();
}
