# Drain

Drain is a process manager like [foreman](https://github.com/ddollar/foreman).

## Build and Test:

```sh
  $ ./bootstrap.sh
```

## Usage

* Create $HOME/.drainfile with the following schema:

  ```
  process-name-1:color-number:group-name-1,group-name-2:command
  process-name-2:color-number:group-name-1:command
  # a comment
  ...
  ```

  * process-name: the user defined name of the process
  * color-number: terminal color number
    (0-255 if your terminal support 256 colors)
  * group-name: simplifies starting a group of processes
    (only the *server* and *up* command supports groups)
  * command: a bourne shell command

* Start all processes with `drain server`.

## Command line

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

## ZSH completion

You can use the sample completion like the
[i project](https://github.com/mbrendler/i/blob/master/zsh-completion/_i).
