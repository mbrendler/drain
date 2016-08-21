#include "config.h"
#include "process_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

int main(int argc, char **argv) {
    signal(SIGWINCH, config_init_term_width);
    config_init();
    ProcessList *l = config_read(CONFIG->drainfile);
    if (!l) {
        fputs("No processes to start\n", stderr);
        return EXIT_FAILURE;
    }

    process_list_process_start(l, argc - 1, argv + 1);

    fd_set set;
    const int max = process_list_max_fd(l, -1);
    while (process_list_init_fd_set(l, &set)) {
        if (-1 == select(max + 1, &set, NULL, NULL, NULL) && EINTR != errno) {
            perror("select");
            return EXIT_FAILURE;
        }
        l = process_list_forward(l, &set);
    }

    process_list_free(l);

    return EXIT_SUCCESS;
}
