#!/bin/sh

PS4="    "

echo "Compiling..."
set -x
g++ src/ltrace_test.cpp -Wall -Wextra -Werror -std=c++20 -O0 -o ltrace_test
{ set +x; } 2>/dev/null
echo "Compiled."
sleep 3

echo ""
echo "Running ltrace_test with ltrace..."
sleep 2
set -x
ltrace -tt -e sin+cos+tan ./ltrace_test
{ set +x; } 2>/dev/null
sleep 2
echo "Finished"
