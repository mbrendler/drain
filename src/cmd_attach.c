#include "process_list.h"
#include "types.h"
#include "actions.h"
#include "client.h"
#include "cmd_server.h"
#include <string.h>
#include <fcntl.h>

int cmd_attach(int argc, char** argv) {
    Message out, in;
    out.nr = mnLog;
    ProcessList *l = NULL;
    for (int i = 0; i < argc; ++i) {
        out.size = strlen(argv[i]) + 1;
        strncpy(out.content, argv[i], sizeof(out.content));
        Client c;
        client_init(&c);
        if (-1 == client_start(&c)) { return -1; }
        if (-1 == client_send(&c, &out)) { return -1; }
        if (-1 == client_receive(&c, &in)) { return -1; }
        if (0 != in.nr) {
            client_stop(&c);
            fprintf(stderr, "process not found - %s\n", argv[i]);
            continue;
        }
        if (-1 == fcntl(c.fd, F_SETFL, fcntl(c.fd, F_GETFL, 0) | O_NONBLOCK)) {
            perror("fcntl");
            return -1;
        }
        Process p;
        process_deserialize(in.content, &p);
        ProcessList *new = process_list_new(argv[i], p.cmd, p.color, c.fd);
        if (new) {
            l = process_list_append(l, &new);
        } else {
            fprintf(stderr, "ignore: %s\n", argv[i]);
        }
    }

    cmd_server_register_signal_handlers();
    const int result = cmd_server_monitor_processes(l, NULL);
    process_list_free(l);
    return result;
}
