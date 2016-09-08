#pragma once

#include "process_list.h"
#include <stdbool.h>

typedef struct {
    const char* termtype;
    unsigned term_width;
    bool line_wrap;
    bool verbose;
    bool keep_running;
    char* drainfile;
    char* socket_path;
} Config;

extern const Config *CONFIG;

void config_init();
void config_init_term_width();
int config_parse_args(int argc, char **argv);
ProcessList* config_read_drainfile(const char* filename);
