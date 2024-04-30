#!/usr/bin/env bash

# Array of trace files
trace_files=(
    "amptjp-bal.rep"
    "cccp-bal.rep"
    "cp-decl-bal.rep"
    "expr-bal.rep"
    "coalescing-bal.rep"
    "random-bal.rep"
    "random2-bal.rep"
    "binary-bal.rep"
    "binary2-bal.rep"
    "realloc-bal.rep"
    "realloc2-bal.rep"
)

# Loop through the array of trace files
for trace_file in "${trace_files[@]}"; do
    echo '--------------------------------'
    echo "Running test for $trace_file"
    ./mdriver -f traces/"$trace_file" -V -l
    echo "Test completed for $trace_file"
    echo "" # Print a blank line for better readability between tests
done

echo "All tests have been completed."
