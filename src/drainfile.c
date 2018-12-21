#include "drainfile.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct {
    char* name;
    int color;
    char* groups;
    char* cmd;
} DrainfileLine;

static DrainfileLine drainfile_parse_line(char* str) {
    DrainfileLine parsed = {
      .name=NULL,
      .color=0,
      .groups=NULL,
      .cmd=NULL
    };
    parsed.name = strsep(&str, ":");
    if (!str) { return parsed; }
    parsed.color = atoi(strsep(&str, ":"));
    if (!str) { return parsed; }
    parsed.groups = strsep(&str, ":");
    if (!str) { return parsed; }
    parsed.cmd = strsep(&str, "\n");
    return parsed;
}

ProcessList* drainfile_read(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        perror("fopen");
        return NULL;
    }
    char buffer[4096];
    ProcessList *l = NULL;
    while (fgets(buffer, sizeof(buffer), f)) {
        if ('#' == *buffer) { continue; } // line is comment
        if ('\n' == *buffer) { continue; } // line is empty
        const DrainfileLine parsed = drainfile_parse_line(buffer);
        if (parsed.name && parsed.cmd) {
            ProcessList *new = process_list_new(
                parsed.name,
                parsed.cmd,
                parsed.color,
                -1,
                parsed.groups
            );
            if (!new) { fprintf(stderr, "Ignore %s\n", parsed.name); }
            l = process_list_append(l, &new);
        } else {
            fprintf(stderr, "parser error in: %s\n", buffer);
        }
    }
    if (ferror(f)) {
        fprintf(
            stderr, "could not load drainfile: %s (%s)",
            filename, strerror(errno)
        );
        process_list_free(l);
        l = NULL;
    }
    fclose(f);
    return l;
}
