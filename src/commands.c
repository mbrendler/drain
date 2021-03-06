#include "commands.h"
#include "cmd_server.h"
#include "cmd_attach.h"
#include "error.h"
#include "config.h"
#include "drainfile.h"
#include "client.h"
#include "helpers.h"
#include "actions.h"
#include "process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>

int cmd_ping(int argc, char** argv) {
  Message msg;
  char const * const content = argc > 0 ? *argv : "hallo";
  memcpy(msg.content, content, strlen(content) + 1);
  const size_t size = strlen(content) + 1;
  if (size > MAX_MESSAGE_CONTENT_SIZE) {
    fprintf(stderr, "resulting message size too long: %zu\n", size);
    return -1;
  }
  msg.size = (uint16_t)size;
  msg.nr = mnPing;

  struct timeval tv1, tv2;
  gettimeofday(&tv1, NULL);
  if (-1 == client_do(&msg, &msg)) { return -1; }
  if (is_error(&msg)) {
    handle_error(&msg);
    return -1;
  }
  gettimeofday(&tv2, NULL);
  const double time = (
    (tv2.tv_usec - tv1.tv_usec) / 1000.0 +
    (tv2.tv_sec - tv1.tv_sec) * 1000.0
  );
  printf("%d bytes time=%.3f ms (%s)\n", msg.size, time, msg.content);
  return 0;
}

int cmd_status(int argc, char** argv) {
  (void)argc;
  (void)argv;
  Message msg = {.nr=mnStatus, .size=0, .content=""};
  if (-1 == client_do(&msg, &msg)) { return -1; }
  if (is_error(&msg)) {
    handle_error(&msg);
    return -1;
  }
  int pos = 0;
  while (pos < msg.size) {
    Process p;
    pos += process_deserialize(msg.content + pos, &p);
    process_print_status(&p);
  }
  return 0;
}

static int simple_command(enum MessageNumber nr, int argc, char** argv) {
  Message msg;
  msg.nr = nr;
  const int size = serialize_string_array(
    argv,
    argc,
    msg.content,
    sizeof(msg.content)
  );
  if (0 > size || size > MAX_MESSAGE_CONTENT_SIZE) {
    fprintf(stderr, "resulting message size too long: %d\n", size);
    return -1;
  }
  msg.size = (uint16_t)size;
  if (-1 == client_do(&msg, &msg)) {
    return -1;
  }
  if (is_error(&msg)) {
    handle_error(&msg);
    return -1;
  }
  return 0;
}

int cmd_up(int argc, char** argv) {
  return simple_command(mnUp, argc, argv);
}

int cmd_halt(int argc, char** argv) {
  return simple_command(mnDown, argc, argv);
}

int cmd_restart(int argc, char** argv) {
  return simple_command(mnRestart, argc, argv);
}

int cmd_add(int argc, char** argv) {
  const bool start = strcmp("-s", argv[0]) == 0;
  if (start) {
    argc--;
    argv++;
  }
  if (argc < 3) { return -1; }
  Message out, in;
  out.nr = mnAdd;
  const int size = serialize_string_array(argv, argc, out.content, sizeof(out.content));
  if (0 > size || size > MAX_MESSAGE_CONTENT_SIZE) {
    fprintf(stderr, "resulting message size too long: %d\n", size);
    return -1;
  }
  out.size = (uint16_t)size;
  replace_char(
    out.content + strlen(argv[0]) + 1 + strlen(argv[1]) + 1,
    out.content + out.size - 1,
    0, ' '
  );
  if (-1 == client_do(&out, &in)) { return -1; }
  if (is_error(&in)) {
    handle_error(&in);
    return -1;
  }
  if (start) {
    return cmd_up(1, argv);
  }
  return 0;
}

int cmd_drainfile(int argc, char** argv) {
  if (argc > 0 && 0 == strcmp("-e", argv[0])) {
    char *editor = getenv("EDITOR");
    if (!editor || !*editor) { editor = "vi"; }
    execlp(editor, editor, CONFIG->drainfile, NULL);
    perror("cmd_drainfile: execlp");
  } else {
    execlp("cat", "cat", CONFIG->drainfile, NULL);
    perror("cmd_drainfile: execlp");
  }
  return 0;
}

static void print_names() {
  ProcessList* l = drainfile_read(CONFIG->drainfile);
  process_list_each(p, l, { puts(p->name); });
  process_list_free(l);
}

int cmd__list_names(int argc, char** argv) {
  (void)argc;
  (void)argv;
  Message msg = {.nr=mnStatus, .size=0, .content=""};
  if (-1 == client_do(&msg, &msg)) {
    print_names();
    return -1;
  }
  if (is_error(&msg)) {
    handle_error(&msg);
    return -1;
  }
  int pos = 0;
  while (pos < msg.size) {
    Process p;
    pos += process_deserialize(msg.content + pos, &p);
    puts(p.name);
  }
  return 0;
}

typedef struct {
  char const * const name;
  const CommandFunction fn;
  char const * const short_help;
} Command;

static const Command COMMANDS[] = {
  { "_list-names", cmd__list_names, "_list-names                       -- list list all available commands"},
  { "up",          cmd_up,          "up [NAME...]                      -- start one, more or all processes" },
  { "status",      cmd_status,      "status                            -- status of drain server" },
  { "server",      cmd_server,      "server [NAME...]                  -- start drain server" },
  { "restart",     cmd_restart,     "restart [NAME...]                 -- restart one, more or all processes" },
  { "ping",        cmd_ping,        "ping                              -- ping drain server" },
  { "help",        cmd_help,        "help [-a]                         -- show this help" },
  { "halt",        cmd_halt,        "halt [NAME...]                    -- stop one, more or all processes" },
  { "drainfile",   cmd_drainfile,   "drainfile [-e]                    -- show / edit drainfile" },
  { "attach",      cmd_attach,      "attach NAME...                    -- retreive output of processes" },
  { "add",         cmd_add,         "add [-s] NAME COLOR CMD [ARGS...] -- add a new process (-s will start it)" },
};

int cmd_help(int argc, char** argv) {
  const bool all = argv && argc > 0 && 0 == strcmp("-a", argv[0]);
  if ((all && argc > 1) || (!all && argc > 0)) { return 0; }
  puts("drain [options] [CMD]\n");
  puts("options");
  puts("  -f FILE    -- configure drainfile path");
  puts("  -S FILE    -- specify full socket path");
  puts("  -h         -- help");
  puts("  -k         -- keep server running");
  puts("  -v         -- verbose");
  puts("  -w         -- line wrapping in log output");
  puts("  -W         -- no line wrapping in log output\n");
  puts("commands");
  const Command *cmd = COMMANDS + sizeof(COMMANDS) / sizeof(*COMMANDS);
  while (COMMANDS != cmd) {
    --cmd;
    if (all || '_' != cmd->name[0]) {
      printf("  %s\n", cmd->short_help);
    }
  }
  return 0;
}

CommandFunction command_get(const char* name) {
  const Command *cmd = COMMANDS + sizeof(COMMANDS) / sizeof(*COMMANDS);
  const Command *found = NULL;
  const size_t name_len = strlen(name);
  bool ambiguous = false;
  while (COMMANDS != cmd) {
    --cmd;
    if (!strncmp(name, cmd->name, name_len)) {
      if (NULL == found) {
        found = cmd;
      } else if (false == ambiguous) {
        ambiguous = true;
        printf("command '%s' is ambiguous:\n", name);
        printf("  %s %s", found->name, cmd->name);
      } else {
        printf(" %s", cmd->name);
      }
    }
  }
  if (NULL == found) {
    printf("unknown command '%s'\n", name);
    return NULL;
  }
  if (false != ambiguous) {
    puts("");
    return NULL;
  }
  return found->fn;
}
