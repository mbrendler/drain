#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

char BUFFER[4096];

ProcessList* config_read(const char* filename) {
    FILE* f = fopen(filename, "r");
    const char *poss[2];
    if (!f) {
        perror("fopen");
        return NULL;
    }
    ProcessList *l = NULL;
    while (fgets(BUFFER, sizeof(BUFFER), f)) {
        if ('#' == *BUFFER) { continue; } // line is comment
        char *r = BUFFER;
        int i = 0;
        while (*r) {
            if (':' == *r && i < 2) {
                *r = '\0';
                poss[i++] = r + 1;
                if (i == 2) { break; }
            }
            ++r;
        }
        l = process_list_append(
            l, process_list_new(BUFFER, poss[1], atoi(poss[0]))
        );
        printf("%s : %s : %s", BUFFER, *poss, poss[1]);
    }
    if (ferror(f)) {
        fprintf(
            stderr, "could not load config file: %s (%s)",
            filename, strerror(errno)
        );
        exit(1);
    }
    fclose(f);
    return l;
}

