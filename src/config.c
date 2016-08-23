#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <termcap.h>

char BUFFER[4096];

enum ConfigLine {clName, clColor, clCmd};

static bool config_parse_line(char* str, const char** cfg) {
    int i = clColor;
    cfg[clName] = str;
    while (*str) {
        if (':' == *str) {
            *str = '\0';
            cfg[i] = str + 1;
            if (clCmd == i) {
                const int cmd_len = strlen(str + 1);
                if (str[cmd_len] == '\n') { str[cmd_len] = 0; }
                return true;
            }
            ++i;
        }
        ++str;
    }
    return false;
}

ProcessList* config_read(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        perror("fopen");
        return NULL;
    }
    ProcessList *l = NULL;
    while (fgets(BUFFER, sizeof(BUFFER), f)) {
        if ('#' == *BUFFER) { continue; } // line is comment
        const char *cfg[3] = {NULL, NULL, NULL};
        if (config_parse_line(BUFFER, cfg)) {
            l = process_list_append(
                l, process_list_new(cfg[clName], cfg[clCmd], atoi(cfg[clColor]))
            );
            printf("%s : %s : %s\n", cfg[clName], cfg[clColor], cfg[clCmd]);
        } else {
            fprintf(stderr, "parser error in: %s\n", BUFFER);
        }
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

static Config CFG = { 80, true, NULL, "drainfile" };
const Config *CONFIG = &CFG;

void config_init() {
    CFG.termtype = getenv("TERM");
    if (tgetent(BUFFER, CFG.termtype) <= 0) {
        CFG.termtype = NULL;
    }
    config_init_term_width();
}

void config_init_term_width() {
    if (CFG.termtype) {
        CFG.term_width = tgetnum("co");
        if (CFG.term_width <= 0) {
            CFG.line_wrap = false;
        }
    }
}
