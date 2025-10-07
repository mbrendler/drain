#pragma once

#include <stdbool.h>

typedef struct {
  const char* termtype;
  char* drainfile;
  char* socket_path;
  int term_width;
  bool line_wrap;
  bool verbose;
  bool keep_running;
  char _reserved;
} Config;

extern const Config *CONFIG;

void config_init(void);
void config_init_term_width(int signum);
int config_parse_args(int argc, char **argv);
