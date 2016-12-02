#include "tests.h"
#include "../src/drainfile.h"
#include <libgen.h>

void test_drainfile_read() {
    char *filename = NULL;
    asprintf(&filename, "%s/drainfile", dirname(__FILE__));
    ProcessList *l = drainfile_read(filename);
    free(filename);
    filename = NULL;
    ASSERT(l);
    ASSERT_STRING("proc-1", l->p.name);
    ASSERT_STRING("echo proc-1", l->p.cmd);
    ASSERT_INT(1, l->p.color);
    ASSERT_STRING("proc-5", l->n->p.name);
    ASSERT_STRING("echo proc-5", l->n->p.cmd);
    ASSERT_INT(5, l->n->p.color);
    ASSERT_POINTER(NULL, l->n->n);
    process_list_free(l);
}

void test_drainfile_read_file_not_exist() {
    char *filename = NULL;
    asprintf(&filename, "%s/not-existing-drainfile", dirname(__FILE__));
    ProcessList *l = drainfile_read(filename);
    free(filename);
    filename = NULL;
    ASSERT(!l)
}

int main() {
    test_drainfile_read();
    test_drainfile_read_file_not_exist();
}
