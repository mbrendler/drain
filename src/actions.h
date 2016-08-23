#pragma once

#include "types.h"

int perform_action(Message* in, Message* out, ProcessList* l);

int perform_command(const char* name, int argc, char** argv);
