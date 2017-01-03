#pragma once

#include "types.h"
#include <stdbool.h>

bool is_error(const Message* msg);

void handle_error(const Message* msg);
