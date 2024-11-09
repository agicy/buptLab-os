#!/bin/sh

PS4="    "

echo "Compiling..."
set -x
g++ src/kill_test.cpp -Wall -Wextra -Werror -std=c++20 -O2 -static -o kill_test
{ set +x; } 2>/dev/null
echo "Compiled."
sleep 3

echo ""
echo "Running kill_test..."
set -x
./kill_test &
{ set +x; } 2>/dev/null
sleep 1

pid=$!
echo ""
echo "kill_test is running, PID = $pid"
sleep 3

echo "Try to terminate kill_test..."
sleep 3
for _ in 1 2 3; do
    sleep 2
    echo ""
    set -x
    kill -15 $pid # SIGTERM
    { set +x; } 2>/dev/null
done

echo ""
echo "Failed to terminate kill_test."

echo ""
echo "Try to kill kill_test..."
sleep 3
set -x
kill -9 $pid # SIGKILL
{ set +x; } 2>/dev/null
echo "Killed"
