#include <stdlib.h>
#include <string.h>

void replace_char(char *str, char *end, char from, char to) {
    while (str < end) {
        if (*str == from) {
            *str = to;
        }
        str++;
    }
}

int serialize_string_array(char** array, int size, char* buffer, int buffer_size) {
    int out_size = 0;
    for (char **end = array + size; array < end; ++array) {
        int len = strlen(*array) + 1;
        if (len > buffer_size) { return -1; }
        memcpy(buffer, *array, len);
        buffer += len;
        out_size += len;
        buffer_size -= len;
    }
    return out_size;
}

int deserialize_string_array(char* strs, int strs_size, char*** array) {
    // !!! FREE MEMORY 'array' !!!
    char *str = strs;
    int count = 0;
    while (str < strs + strs_size) {
        str += strlen(str) + 1;
        count++;
    }
    *array = calloc(count, sizeof(char*));
    str = strs;
    for (int i = 0; i < count; ++i) {
        (*array)[i] = str;
        str += strlen(str) + 1;
    }
    return count;
}
