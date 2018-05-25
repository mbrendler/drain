#include "tests.h"
#include "../src/drainfile.h"
#include <libgen.h>

char* here() { // with gcc in linux dirname(__FILE__) does not work
    static char *this_filename = NULL;
    if (!this_filename) {
        asprintf(&this_filename, "%s", __FILE__);
    }
    return dirname(this_filename);
}

void test_drainfile_read() {
    char *filename = NULL;
    asprintf(&filename, "%s/drainfile", here());
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
    ASSERT_STRING("proc-6", l->n->n->p.name);
    ASSERT_STRING("echo proc-6", l->n->n->p.cmd);
    ASSERT_INT(6, l->n->n->p.color);
    ASSERT_POINTER(NULL, l->n->n->n);
    process_list_free(l);
}

void test_drainfile_read_file_not_exist() {
    char *filename = NULL;
    asprintf(&filename, "%s/not-existing-drainfile", here());
    ProcessList *l = drainfile_read(filename);
    free(filename);
    filename = NULL;
    ASSERT(!l)
}

int main() {
    test_drainfile_read();
    test_drainfile_read_file_not_exist();
}
