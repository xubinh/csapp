#!/usr/bin/env bash

error_msg='Invalid input. Please enter test type (test/rtest) and a number between 1 and 16 for one test or '"'"'all'"'"' to run all tests.'

test_type="$1"
test_choice="$2"

if [[ $test_type != "test" && $test_type != "rtest" ]]; then
    echo "$error_msg"
fi

output_file='output_'"$test_type"'.txt'

if [[ $test_choice == "all" ]]; then
    true >"$output_file"

    for i in $(seq 1 16); do
        if [[ $i -lt 10 ]]; then
            i="0$i"
        fi

        make "$test_type$i" 2>&1 | tee -a "$output_file"
    done
else
    if [[ $test_choice =~ ^[1-9]$|^1[0-6]$ ]]; then
        make "$test_type$test_choice" >"$output_file" 2>&1
    else
        echo "$error_msg"
    fi
fi
