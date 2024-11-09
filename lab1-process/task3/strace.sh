#!/bin/sh

PS4="    "

echo "Compiling..."
set -x
g++ src/strace_test.cpp -Wall -Wextra -Werror -std=c++20 -O2 -static -o strace_test
{ set +x; } 2>/dev/null
echo "Compiled."
sleep 3

echo ""
echo "Running strace_test with strace..."
sleep 2
set -x
strace -c -e fault=getpid ./strace_test
{ set +x; } 2>/dev/null
sleep 2
echo "Finished"
