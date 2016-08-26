#include "config.h"
#include "commands.h"
#include <string.h>
#include <signal.h>

int main(int argc, char **argv) {
    config_init();
    signal(SIGWINCH, config_init_term_width);

    const char *cmd = argc > 1 ? argv[1] : "help";
    return perform_command(cmd, argc > 1 ? argc - 2 : 0, argv + 2);
}
