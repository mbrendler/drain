#include "actions.h"
#include <stdio.h>

#include <string.h>
int action_ping(int in_len, char* in, int* out_len, char* out) {
    memcpy(out, in, in_len);
    *out_len = in_len;
    return 0;
}

typedef int(*ActionFunction)(int, char*, int*, char*);

typedef struct {
    const char* name;
    ActionFunction fn;
} Action;

const Action ACTIONS[] = {
    { "ping", action_ping }
};

int perform_action(unsigned nr, int size, char* in, int *out_size, char* out) {
    if (nr < sizeof(ACTIONS) / sizeof(*ACTIONS)) {
        printf("call action: %s\n", ACTIONS[nr].name);
        return ACTIONS[nr].fn(size, in, out_size, out);
    }
    return -1;
}
