#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define ASSERT(x) if (!(x)) {                                                  \
    printf("failure: %s:%d  %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__); \
    exit(EXIT_FAILURE);                                                      \
  }

#define ASSERT_STRING(expect, value) if (strcmp(expect, value)) {          \
    printf("failure: %s:%d  %s\n    expected: '%s'\n         got: '%s'\n", \
      __FILE__, __LINE__, __PRETTY_FUNCTION__, expect, value);             \
    exit(EXIT_FAILURE);                                                    \
  }

#define ASSERT_BYTES(size, expect, value) if (memcmp(expect, value, size)) {                          \
    printf("failure: %s:%d  %s\n    expected: ",                                                      \
      __FILE__, __LINE__, __PRETTY_FUNCTION__);                                                       \
    for (size_t i = 0; i < size ; ++i) { printf(isprint(expect[i]) ? " %c," : "0x%2.2x,", expect[i]); } \
    printf("\n         got: ");                                                                       \
    for (size_t i = 0; i < size ; ++i) { printf(isprint(value[i]) ? " %c," : "0x%2.2x,", value[i]); }   \
    puts("");                                                                                         \
  }

#define ASSERT_INT(expect, value) if (expect != value) {               \
    printf("failure: %s:%d  %s\n    expected: %d\n         got: %d\n", \
      __FILE__, __LINE__, __PRETTY_FUNCTION__, expect, value);         \
    exit(EXIT_FAILURE);                                                \
  }

#define ASSERT_POINTER(expect, value) if (expect != value) {                 \
    printf("failure: %s:%d  %s\n    expected: %p\n         got: %p\n",       \
      __FILE__, __LINE__, __PRETTY_FUNCTION__, (void*)expect, (void*)value); \
    exit(EXIT_FAILURE);                                                      \
  }
