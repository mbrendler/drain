#pragma once

#include "types.h"

ProcessList* process_list();

void cmd_server_register_signal_handlers();

int cmd_server_monitor_processes(ProcessList* l, Server* s);

int cmd_server(int argc, char **argv);
