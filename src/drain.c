#include "config.h"
#include "process_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int main() {
    ProcessList *l = config_read("drainfile");
    if (!l) {
        fputs("No processes to start\n", stderr);
        return EXIT_FAILURE;
    }

    fd_set set;
    const int max = process_list_max_fd(l, -1);
    while (l) {
        process_list_init_fd_set(l, &set);
        if (-1 == select(max + 1, &set, NULL, NULL, NULL) && EINTR != errno) {
            perror("select");
            return EXIT_FAILURE;
        }
        l = process_list_forward(l, &set);
    }

    process_list_free(l);

    return EXIT_SUCCESS;
}
