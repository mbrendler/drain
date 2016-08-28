#pragma once

#include "types.h"

enum MessageNumber {
    mnPing, mnStatus, mnUp, mnDown, mnRestart, mnAdd,
};

int perform_action(int fd, Message* in, Message* out, ProcessList* l);
