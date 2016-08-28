#include "tests.h"
#include "../src/config.h"

void test_config_parse_args() {
    char *argv[] = {"program-name", "-v", "-W", "some", "other", "args"};
    int new_argc = config_parse_args(6, argv);
    ASSERT_INT(3, new_argc);
    ASSERT_INT(true, CONFIG->verbose);
    ASSERT_INT(false, CONFIG->line_wrap);
}

int main() {
    test_config_parse_args();
    return 0;
}
