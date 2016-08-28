#include "tests.h"
#include "../src/config.h"

void test_config_parse_args() {
    char *argv[] = {"program-name", "-v", "-W", "-k", "some", "other", "args"};
    int new_argc = config_parse_args(7, argv);
    ASSERT_INT(3, new_argc);
    ASSERT_INT(true, CONFIG->verbose);
    ASSERT_INT(false, CONFIG->line_wrap);
    ASSERT_INT(true, CONFIG->keep_running);
}

int main() {
    test_config_parse_args();
    return 0;
}
