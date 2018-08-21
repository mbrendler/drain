# Drain

## Build and Test:

```sh
  $ ./bootstrap.sh
```

## Usage

Create $HOME/.drainfile with the following schema:

```
process1:color:group-1,group-2:command
process2:color:group-1,group-2:command
...
```

Start all processes with `drain server`.

Commandline options:
```
drain [options] [CMD]

options
  -f FILE    -- configure drainfile path
  -S FILE    -- specify full socket path
  -h         -- help
  -k         -- keep server running
  -v         -- verbose
  -w         -- line wrapping in log output
  -W         -- no line wrapping in log output

commands
  add [-s] NAME COLOR CMD [ARGS...] -- add a new process (-s will start it)
  attach NAME...                    -- retreive output of processes
  drainfile [-e]                    -- show / edit drainfile
  halt [NAME...]                    -- stop one, more or all processes
  help [-a]                         -- show this help
  ping                              -- ping drain server
  restart [NAME...]                 -- restart one, more or all processes
  server [NAME...]                  -- start drain server
  status                            -- status of drain server
  up [NAME...]                      -- start one, more or all processes
```
