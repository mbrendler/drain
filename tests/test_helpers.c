#include "tests.h"
#include "../src/helpers.h"

void test_replace_char() {
    char str[] = "hallo\0welt\0wald";
    char *end = str + sizeof(str) - 1; // -1 .. stop before last '\0'
    replace_char(str, end, '\0', ' ');
    ASSERT_STRING("hallo welt wald", str);
}

void test_serialize_string_array() {
    char *array[] = {"hallo", "welt", "wald"};
    char buffer[16];

    int size = serialize_string_array(array, sizeof(array) / sizeof(*array), buffer, sizeof(buffer));
    ASSERT_INT(16, size);
    ASSERT_BYTES(sizeof(buffer), "hallo\0welt\0wald\0", buffer);

    // Buffer is too small:
    size = serialize_string_array(array, sizeof(array) / sizeof(*array), buffer, sizeof(buffer) - 1);
    ASSERT_INT(-1, size);
}

void test_deserialize_string_array() {
    char strs[] = "hallo\0welt\0wald";
    char **array = NULL;
    int count = deserialize_string_array(strs, sizeof(strs), &array);
    ASSERT_INT(3, count);
    ASSERT_STRING("hallo", array[0]);
    ASSERT_STRING("welt", array[1]);
    ASSERT_STRING("wald", array[2]);
    // the input is not modufied
    ASSERT_BYTES(sizeof(strs), "hallo\0welt\0wald\0", strs);
    free(array);
}

int main() {
    test_replace_char();
    test_serialize_string_array();
    test_deserialize_string_array();
    return EXIT_SUCCESS;
}
