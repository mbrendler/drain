#pragma once

#include "process_list.h"
#include <stdbool.h>

ProcessList* config_read(const char* filename);

typedef struct {
    unsigned term_width;
    bool line_wrap;
    bool verbose;
    bool keep_running;
    const char* termtype;
    const char* drainfile;
} Config;

extern const Config *CONFIG;

void config_init();
void config_init_term_width();
int config_parse_args(int argc, char **argv);
