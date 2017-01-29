#! /usr/bin/env bash

set -euo pipefail

if test "${DEBUG+x}" ; then
  set -x
fi

readonly here="$(dirname "$(readlink -m "$0")")"
readonly drainfile="$here/../drainfile"
readonly socket="/tmp/_drain_leak_test"
readonly drain="$1/drain"

function run-valgrind() {
  valgrind --leak-check=full --track-origins=yes "$@"
}

function run-drain() {
  echo "$drain" -f "$drainfile" -S "$socket" "$@" >&2
  if which valgrind >/dev/null 2>/dev/null ; then
    local out;out="$(run-valgrind "$drain" -f "$drainfile" -S "$socket" "$@" 2>&1)"
    # ERROR SUMMARY: 1 errors from 1 contexts (suppressed: 0 from 0)
    if [[ ! "$out" =~ "ERROR SUMMARY: 0" ]] ; then
      echo FAILED >&2
      echo "$out" >&2
      exit 1
    fi
    echo "$out"
  else
    "$drain" -f "$drainfile" -S "$socket" "$@" 2>&1
  fi
}

function start-server() {
  run-drain server &
}

function stop-server() {
  run-drain halt > /dev/null || true
  for job in $(jobs -p) ; do
    wait "$job"
  done
}

function wait-settled() {
  while true ; do
    sleep 0.1
    local st;st="$( ("$drain" -f "$drainfile" -S "$socket" status || true) | tr -d \  | awk -F'|' '{ print $2 }')"
    local expect=$"down
down
up"
    test "$st" = "$expect" && break
  done
}

function fail() {
  echo "got: $2"
  echo "expected: $1"
  exit 1
}

function assert-includes() {
  local got;got="$(cat)"
  for x in "$@" ; do
    if ! grep "$x" <<<"$got" >/dev/null 2>/dev/null ; then
      fail "$x" "$got"
    fi
  done
}

function assert-equal() {
  local got;got="$(cat)"
  local expected="$1"
  if [ "$expected" != "$got" ] ; then
    fail "$expected" "$got"
  fi
}

function trap-function() {
  stop-server
  rm -f /tmp/_drain_test_attach_out
}

trap trap-function EXIT

# Tests

echo Test drainfile command ---------------------------------------------------
"$drain" -f "$drainfile" drainfile | assert-equal "$(cat "$drainfile")"

echo Test help command --------------------------------------------------------
run-drain help | assert-includes add attach drainfile help ping status

echo Test halt command --------------------------------------------------------
start-server
wait-settled
stop-server
if [ -e "$socket" ] ; then
  echo Drain does not stop correctly
  exit 1
fi

echo Test ping command --------------------------------------------------------
start-server
wait-settled
run-drain ping 'hallo welt' | assert-includes "(hallo welt)"
stop-server

echo Test status command ------------------------------------------------------
start-server
wait-settled
run-drain status | assert-includes "tail.*down" "ls.*down" "tic.*up"
stop-server

echo Test up command ----------------------------------------------------------
up_server_output=$(
  start-server
  wait-settled
  run-drain up ls
  wait-settled
  stop-server
)
echo "$up_server_output"
if [ "$(grep -c "ls:.*_drain_leak_test" <<<"$up_server_output")" -ne 2 ] ; then
  fail "2" "$(grep -c "ls:.*_drain_leak_test" <<<"$up_server_output")"
fi

echo Test restart command -----------------------------------------------------
start-server
wait-settled
tic_pid_before_restart="$(run-drain status | tr -d \  | grep tic | cut -d\| -f3)"
run-drain restart tic
wait-settled
tic_pid_after_restart="$(run-drain status | tr -d \  | grep tic | cut -d\| -f3)"
if [ "$tic_pid_before_restart" = "$tic_pid_after_restart" ] ; then
  echo "should be differnt"
  fail "$tic_pid_before_restart" "$tic_pid_after_restart"
fi
stop-server

echo Test add command ---------------------------------------------------------
start-server
wait-settled
run-drain add blubber 4 echo hallo
run-drain status | assert-includes 'blubber.*down.*echo hallo'
stop-server

echo Test add -s command ---------------------------------------------------------
(
  start-server
  wait-settled
  run-drain add -s blubber 4 echo hallo welt wald
  run-drain status | assert-includes 'blubber.*echo hallo welt wald'
  stop-server
) | assert-includes "blubber:.*hallo welt wald"

echo Test attach command ------------------------------------------------------
start-server
wait-settled
run-drain attach tic >> /tmp/_drain_test_attach_out &
sleep 6
stop-server
assert-includes 'tic:' 'process stopped: tic' < /tmp/_drain_test_attach_out
