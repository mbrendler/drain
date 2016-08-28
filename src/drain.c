#include "config.h"
#include "commands.h"
#include <string.h>
#include <signal.h>

int main(int argc, char **argv) {
    config_init();
    const int new_argc = config_parse_args(argc, argv);
    if (-1 == new_argc) {
        return -1;
    }
    signal(SIGWINCH, config_init_term_width);

    argv += argc - new_argc;
    const char *cmd = new_argc > 0 ? argv[0] : "help";
    return perform_command(cmd, new_argc > 0 ? new_argc - 1 : 0, argv + 1);
}
