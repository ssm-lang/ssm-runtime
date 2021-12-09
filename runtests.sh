#!/usr/bin/env bash

set -euf
set -o pipefail

BUILD_DIR=./build

scriptname="$(basename "$0")"

say () {
  echo "[$scriptname]" "$@"
}

# Run an example
run () {
  local exe="$BUILD_DIR/$1"
  shift
  echo "$exe" "$@"
  "$exe" "$@" 2>&1 | sed 's/^/# /'
}

vg () {
  local exe="$BUILD_DIR/$1"
  shift
  echo "$exe" "$@"
  valgrind "$exe" "$@" 2>&1 | sed 's/^/# /'
}

make exes tests

if ! [ -d "$BUILD_DIR" ] ; then
  echo "Could not find build directory: $BUILD_DIR. Quitting."
  exit 1
fi

rm -f build/examples.out
{
  run fib
  run fib 4
  run fib 5
  run fib 13
  run fib 15
  run onetwo
  run clock
  run counter
} >> build/examples.out

if diff build/examples.out test/examples.out &> build/examples.diff ; then
  say "Example output matches expected."
else
  say "Example output differs from expected:"
  cat build/examples.diff
  exit 1
fi

rm -f build/tests.out
{
  run test_test-scheduler
} >> build/tests.out

if diff build/tests.out test/tests.out &> build/tests.diff ; then
  say "Test output matches expected."
else
  say "Test output differs from expected:"
  cat build/tests.diff
  exit 1
fi

if command -v valgrind >/dev/null ; then
  rm -f build/examples.vg-out
  {
    vg fib
    vg fib 4
    vg fib 5
    vg fib 13
    vg fib 15
    vg onetwo
    vg clock
    vg counter
  } >> build/examples.vg-out
  say "Examples do not have any memory errors"

  {
    vg test_test-scheduler
  } >> build/tests.out
  say "Tests do not have any memory errors"
else
  say "Warning: valgrind not found in PATH. Skipping memory tests."
fi

make cov
