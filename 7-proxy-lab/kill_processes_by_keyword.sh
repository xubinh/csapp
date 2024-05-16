#!/usr/bin/env bash

function kill_processes_by_keyword() {
    keyword=$1

    # Find the PIDs of processes with the keyword in their command line
    pids=$(ps aux | grep "$keyword" | grep -v grep | awk '{print $2}')
    # echo "$pids"

    # Check if there are any PIDs found
    if [ -z "$pids" ]; then
        # echo "No processes found with the keyword: $keyword"
        exit 0
    fi

    # Kill the processes
    for pid in $pids; do
        echo "Killing process with PID: $pid"
        kill -9 "$pid"
    done

    # echo "All processes with the keyword '$keyword' have been killed."
}

kill_processes_by_keyword "$1"
