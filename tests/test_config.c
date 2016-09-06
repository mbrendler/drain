#include "tests.h"
#include "../src/config.h"

void test_config_init() {
    config_init();
    ASSERT_STRING(getenv("TERM"), CONFIG->termtype);

    char *expected_drainfile = NULL;
    asprintf(&expected_drainfile, "%s/.drainfile", getenv("HOME"));
    ASSERT_STRING(expected_drainfile, CONFIG->drainfile);
    free(expected_drainfile);
    expected_drainfile = NULL;
}

void test_config_parse_args() {
    char *argv[] = {
        "program-name",
        "-v", "-W", "-k", "-f", "another-drainfile",
        "some", "other", "args"
    };
    int new_argc = config_parse_args(9, argv);
    ASSERT_INT(3, new_argc);
    ASSERT_INT(true, CONFIG->verbose);
    ASSERT_INT(false, CONFIG->line_wrap);
    ASSERT_INT(true, CONFIG->keep_running);
    ASSERT_STRING("another-drainfile", CONFIG->drainfile);
}

int main() {
    test_config_init();
    test_config_parse_args();
}
