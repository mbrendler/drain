#include "config.h"
#include "commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termcap.h>

typedef struct {
    char* name;
    int color;
    char* cmd;
} DrainfileLine;

static DrainfileLine config_parse_drainfile_line(char* str) {
    DrainfileLine parsed = {NULL, 0, NULL};
    parsed.name = strsep(&str, ":");
    if (!str) { return parsed; }
    parsed.color = atoi(strsep(&str, ":"));
    if (!str) { return parsed; }
    parsed.cmd = strsep(&str, "\n");
    return parsed;
}

ProcessList* config_read_drainfile(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        perror("fopen");
        return NULL;
    }
    char buffer[4096];
    ProcessList *l = NULL;
    while (fgets(buffer, sizeof(buffer), f)) {
        if ('#' == *buffer) { continue; } // line is comment
        const DrainfileLine parsed = config_parse_drainfile_line(buffer);
        if (parsed.name && parsed.cmd) {
            ProcessList *new = process_list_new(
                parsed.name, parsed.cmd, parsed.color, -1
            );
            if (!new) { fprintf(stderr, "Ignore %s\n", parsed.name); }
            l = process_list_append(l, &new);
            printf("%s : %d : %s\n", parsed.name, parsed.color, parsed.cmd);
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

static Config CFG = {
    .termtype = NULL,
    .term_width = 80,
    .line_wrap = true,
    .verbose = false,
    .keep_running = false,
    .drainfile = NULL,
};
const Config *CONFIG = &CFG;

void config_init() {
    CFG.termtype = getenv("TERM");
    // Will only be freed, if the command-line argument -f is given:
    if (-1 == asprintf(&CFG.drainfile, "%s/.drainfile", getenv("HOME"))) {
        perror("config: asprintf drainfile");
        exit(1);
    }
    config_init_term_width();
}

void config_init_term_width() {
    if (CFG.termtype) {
        char buffer[4096];
        if (1 != tgetent(buffer, CFG.termtype)) {
            fprintf(stderr,
                "could not load term entry for '%s' - use line witdh of %d\n",
                CFG.termtype, CFG.term_width
            );
            return;
        }
        CFG.term_width = tgetnum("co");
        if (CFG.term_width <= 0) {
            CFG.line_wrap = false;
        }
    }
}

int config_parse_args(int argc, char **argv) {
    int ch;
    while (-1 != (ch = getopt(argc, argv, "vwWkf:h"))) {
        switch (ch) {
            case 'v': CFG.verbose = true; break;
            case 'w': CFG.line_wrap = true; break;
            case 'W': CFG.line_wrap = false; break;
            case 'k': CFG.keep_running = true; break;
            case 'h': cmd_help(0, NULL); exit(0); break;
            case 'f':
                if (NULL != CFG.drainfile) { free(CFG.drainfile); }
                CFG.drainfile = optarg;
                break;
            default: return -1;
        }
    }
    return argc - optind;
}
