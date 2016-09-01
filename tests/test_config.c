#include "tests.h"
#include "../src/config.h"

void test_config_init() {
    config_init();
    ASSERT_STRING(getenv("TERM"), CONFIG->termtype);

    char *expected_drainfile = malloc(strlen(getenv("HOME")) + sizeof(".drainfile"));
    strcpy(expected_drainfile, getenv("HOME"));
    strcpy(expected_drainfile + strlen(getenv("HOME")), ".drainfile");
    ASSERT_STRING("/Users/mbrendler/.drainfile", CONFIG->drainfile);
    free(expected_drainfile);
    expected_drainfile = NULL;
}

void test_config_parse_args() {
    char *argv[] = {"program-name", "-v", "-W", "-k", "some", "other", "args"};
    int new_argc = config_parse_args(7, argv);
    ASSERT_INT(3, new_argc);
    ASSERT_INT(true, CONFIG->verbose);
    ASSERT_INT(false, CONFIG->line_wrap);
    ASSERT_INT(true, CONFIG->keep_running);
}

int main() {
    test_config_init();
    test_config_parse_args();
    return 0;
}
