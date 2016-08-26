#pragma once

void replace_char(char *str, char *end, char from, char to);

int serialize_string_array(char** array, int size, char* buffer, int buffer_size);

// free string array
int deserialize_string_array(char* strs, int strs_size, char*** array);
