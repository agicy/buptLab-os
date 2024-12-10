#!/bin/sh

echo "Enter the program name: "
read -r program_name

program_dir="build"
output_dir="logs"

program="${program_dir}/${program_name}"
output_file="${output_dir}/${program_name}.log"

mkdir -p logs
rm -f "${output_file}"

i=1
while [ $i -le 5 ]; do
    nice -n -20 "${program}" >>"${output_file}"
    # sleep 1
    echo "Program ${program_name} has been executed for ${i} times."
    i=$((i + 1))
done
