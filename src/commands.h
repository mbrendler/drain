#pragma once

typedef int(*CommandFunction)(int, char**);

CommandFunction command_get(const char* name);

int cmd_help(int argc, char** argv);
