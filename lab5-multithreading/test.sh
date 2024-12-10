#!/bin/sh

find build -type f -exec basename {} \; | while read -r filename; do
    echo "Processing file: $filename"
    sleep 1
    echo "$filename" | ./run.sh
done
