#include "config.h"
#include "commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termcap.h>

static Config CFG = {
    .termtype = NULL,
    .term_width = 80,
    .line_wrap = true,
    .verbose = false,
    .keep_running = false,
    .drainfile = NULL,
    .socket_path = NULL,
};
const Config *CONFIG = &CFG;

void config_init() {
    CFG.termtype = getenv("TERM");
    // Will only be freed, if the command-line argument -f is given:
    if (-1 == asprintf(&CFG.drainfile, "%s/.drainfile", getenv("HOME"))) {
        perror("config: asprintf drainfile");
        exit(1);
    }
    if (-1 == asprintf(&CFG.socket_path, "/tmp/drain-%d", getuid())) {
        perror("config: asprintf socket_path");
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
    while (-1 != (ch = getopt(argc, argv, "+vwWkf:S:h"))) {
        switch (ch) {
            case 'v': CFG.verbose = true; break;
            case 'w': CFG.line_wrap = true; break;
            case 'W': CFG.line_wrap = false; break;
            case 'k': CFG.keep_running = true; break;
            case 'h': cmd_help(0, NULL); exit(0);
            case 'f':
                if (NULL != CFG.drainfile) { free(CFG.drainfile); }
                CFG.drainfile = optarg;
                break;
            case 'S':
                if (NULL != CFG.socket_path) { free(CFG.socket_path); }
                CFG.socket_path = optarg;
                break;
            default: return -1;
        }
    }
    return argc - optind;
}
