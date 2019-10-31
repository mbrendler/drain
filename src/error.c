#include "error.h"
#include <stdio.h>

bool is_error(const Message* msg) {
  return msg->nr < 0;
}

void handle_error(const Message* msg) {
  if (-1 == msg->nr) {
    fprintf(stderr, "%s\n", msg->content);
  } else {
    fprintf(stderr, "Unknown error number: %d\n", msg->nr);
  }
}
