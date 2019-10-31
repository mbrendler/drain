#include "tests.h"
#include "../src/config.h"
#include <unistd.h>

void test_config_init() {
    config_init();
    const char* term = getenv("TERM");
    if (term) {
      ASSERT_STRING(term, CONFIG->termtype);
    }

    char *expected_drainfile = NULL;
    asprintf(&expected_drainfile, "%s/.drainfile", getenv("HOME"));
    ASSERT_STRING(expected_drainfile, CONFIG->drainfile);
    free(expected_drainfile);
    expected_drainfile = NULL;

    char *expected_socket_path = NULL;
    asprintf(&expected_socket_path, "/tmp/drain-%d", getuid());
    ASSERT_STRING(expected_socket_path, CONFIG->socket_path);
    free(expected_socket_path);
    expected_socket_path = NULL;
}

void test_config_parse_args() {
    const int new_argc = config_parse_args(11, (char*[]){
        "program-name",
        "-v", "-W", "-k",
        "-f", "another-drainfile",
        "-S", "another-socket-path",
        "some", "other", "args"
    });
    ASSERT_INT(3, new_argc);
    ASSERT_INT(true, CONFIG->verbose);
    ASSERT_INT(false, CONFIG->line_wrap);
    ASSERT_INT(true, CONFIG->keep_running);
    ASSERT_STRING("another-drainfile", CONFIG->drainfile);
    ASSERT_STRING("another-socket-path", CONFIG->socket_path);
}

int main() {
    test_config_init();
    test_config_parse_args();
}
