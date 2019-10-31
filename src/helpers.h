#pragma once

#include <stddef.h>

void replace_char(char *str, char *end, char from, char to);

int serialize_string_array(
  char** array,
  int size,
  char* buffer,
  size_t buffer_size
);

// free string array
int deserialize_string_array(char* strs, int strs_size, char*** array);

#define MAX(A, B)(A > B ? A : B)
