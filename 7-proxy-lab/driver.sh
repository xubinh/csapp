#!/bin/bash
#
# driver.sh - This is a simple autograder for the Proxy Lab. It does
#     basic sanity checks that determine whether or not the code
#     behaves like a concurrent caching proxy.
#
#     David O'Hallaron, Carnegie Mellon University
#     updated: 2/8/2016
#
#     usage: ./driver.sh
#

# Point values
MAX_BASIC=40
MAX_CONCURRENCY=15
MAX_CACHE=15

# Various constants
HOME_DIR=$(pwd)
PROXY_DIR="./.proxy"
NOPROXY_DIR="./.noproxy"
TIMEOUT=5
MAX_RAND=63000
PORT_START=1024
PORT_MAX=65000
MAX_PORT_TRIES=10

# List of text and binary files for the basic test
BASIC_LIST="home.html
            csapp.c
            tiny.c
            godzilla.jpg
            tiny"

# List of text files for the cache test
CACHE_LIST="tiny.c
            home.html
            csapp.c"

# The file we will fetch for various tests
FETCH_FILE="home.html"

#####
# Helper functions
#

#
# download_proxy - download a file from the origin server via the proxy
# usage: download_proxy <testdir> <filename> <origin_url> <proxy_url>
#
function download_proxy {
    cd "$1" || exit
    curl --max-time ${TIMEOUT} --silent --proxy "$4" --output "$2" "$3"
    (($? == 28)) && echo "Error: Fetch timed out after ${TIMEOUT} seconds"
    cd "$HOME_DIR" || exit
}

#
# download_noproxy - download a file directly from the origin server
# usage: download_noproxy <testdir> <filename> <origin_url>
#
function download_noproxy {
    cd "$1" || exit
    curl --max-time ${TIMEOUT} --silent --output "$2" "$3"
    (($? == 28)) && echo "Error: Fetch timed out after ${TIMEOUT} seconds"
    cd "$HOME_DIR" || exit
}

#
# clear_dirs - Clear the download directories
#
function clear_dirs {
    rm -rf "${PROXY_DIR:?}"/*
    rm -rf "${NOPROXY_DIR:?}"/*
}

#
# free_port - returns an available unused TCP port
#
function free_port {
    # Generate a random port in the range [PORT_START,
    # PORT_START+MAX_RAND]. This is needed to avoid collisions when many
    # students are running the driver on the same machine.

    # 随机抽取一个整数作为预定端口号:
    port=$(((RANDOM % MAX_RAND) + PORT_START))

    while true; do
        # 通过处理 `netstat` 的输出得到正在使用的 TCP 端口:
        ports_in_use=$(
            netstat --numeric-ports --numeric-hosts -a --protocol=tcpip |
                grep tcp |
                cut -c21- |
                cut -d':' -f2 |
                cut -d' ' -f1 |
                grep -E "[0-9]+" |
                uniq |
                tr "\n" " "
        )

        # 如果预定端口正在使用:
        if echo "${ports_in_use}" | grep -wq "${port}"; then
            # 如果已经超出最大可用端口号范围:
            if [ "$port" -eq ${PORT_MAX} ]; then
                # 返回 -1:
                echo "-1"

                return
            fi

            # 否则加 1 并继续测试可用性:
            port=$(("${port}" + 1))

            continue

        # 如果预定端口空闲:
        else
            # 返回该端口号:
            echo "${port}"

            return
        fi
    done
}

#
# wait_for_port_use - Spins until the TCP port number passed as an
#     argument is actually being used. Times out after 5 seconds.
#
function wait_for_port_use() {
    timeout=1
    timeout_count=0

    # 如果传入的端口不在已使用端口列表中:
    while true; do
        # 使用 `netstat` 输出当前已建立连接的 TCP 端口号:
        portsinuse=$(
            netstat --numeric-ports --numeric-hosts -a --protocol=tcpip |
                grep tcp |
                cut -c21- |
                cut -d':' -f2 |
                cut -d' ' -f1 |
                grep -E "[0-9]+" |
                uniq |
                tr "\n" " "
        )

        # 如果已经建立起连接:
        if echo "${portsinuse}" | grep -wq "$1"; then
            # 退出循环:
            break
        fi

        timeout_count=$(("${timeout_count}" + 1))

        # 如果重试次数超过最大允许重试次数:
        if [ "${timeout_count}" == "${MAX_PORT_TRIES}" ]; then
            # 直接报错:
            kill -ALRM $$
        fi

        # 否则等待若干秒:
        echo "Sleep for $timeout seconds waiting for port to be used..."
        sleep "$timeout"
    done
}

#######
# Main
#######

######
# Verify that we have all of the expected files with the right
# permissions
#

make clean &>/dev/null

# Kill any stray proxies or tiny servers owned by this user
# killall -q proxy tiny nop-server.py 2>/dev/null
./kill_process.sh &>/dev/null

make &>/dev/null

# Make sure we have a Tiny directory
if [ ! -d ./tiny ]; then
    echo "Error: ./tiny directory not found."
    exit
fi

# If there is no Tiny executable, then try to build it
if [ ! -x ./tiny/tiny ]; then
    echo "Building the tiny executable."
    (
        cd "./tiny" || exit
        make &>/dev/null
    )
    echo ""
fi

# Make sure we have all the Tiny files we need
if [ ! -x ./tiny/tiny ]; then
    echo "Error: ./tiny/tiny not found or not an executable file."
    exit
fi
for file in ${BASIC_LIST}; do
    if [ ! -e "./tiny/${file}" ]; then
        echo "Error: ./tiny/${file} not found."
        exit
    fi
done

# Make sure we have an existing executable proxy
if [ ! -x ./proxy ]; then
    echo "Error: ./proxy not found or not an executable file. Please rebuild your proxy and try again."
    exit
fi

# Make sure we have an existing executable nop-server.py file
if [ ! -x ./nop-server.py ]; then
    echo "Error: ./nop-server.py not found or not an executable file."
    exit
fi

# Create the test directories if needed
if [ ! -d ${PROXY_DIR} ]; then
    mkdir ${PROXY_DIR}
fi

if [ ! -d ${NOPROXY_DIR} ]; then
    mkdir ${NOPROXY_DIR}
fi

# Add a handler to generate a meaningful timeout message
trap 'echo "Timeout waiting for the server to grab the port reserved for it"; kill $$' ALRM

#####
# Basic
#
echo "*** Basic ***"

# Run the Tiny Web server
tiny_port=$(free_port)
echo "Starting tiny on ${tiny_port}"
cd "./tiny" || exit
./tiny "${tiny_port}" &>/dev/null &
# ./tiny "${tiny_port}" &
tiny_pid=$!
cd "${HOME_DIR}" || exit

# echo "Sleep for 5 seconds for TINY to establish..."
# sleep 5

# Wait for tiny to start in earnest
wait_for_port_use "${tiny_port}"

# Run the proxy
proxy_port=$(free_port)
echo "Starting proxy on ${proxy_port}"
./proxy "${proxy_port}" &>/dev/null &
# ./proxy "${proxy_port}" &
proxy_pid=$!

# Wait for the proxy to start in earnest
wait_for_port_use "${proxy_port}"

# Now do the test by fetching some text and binary files directly from
# Tiny and via the proxy, and then comparing the results.
numRun=0
numSucceeded=0
for file in ${BASIC_LIST}; do
    numRun=$(("$numRun" + 1))
    echo "${numRun}: ${file}"

    # Fetch directly from Tiny
    echo "   Fetching ./tiny/${file} into ${NOPROXY_DIR} directly from Tiny"
    download_noproxy $NOPROXY_DIR "${file}" "http://localhost:${tiny_port}/${file}"

    # Fetch using the proxy
    echo "   Fetching ./tiny/${file} into ${PROXY_DIR} using the proxy"
    download_proxy $PROXY_DIR "${file}" "http://localhost:${tiny_port}/${file}" "http://localhost:${proxy_port}"

    # Compare the two files
    echo "   Comparing the two files"
    if diff -q "${PROXY_DIR}/${file}" "${NOPROXY_DIR}/${file}" &>/dev/null; then
        numSucceeded=$(("${numSucceeded}" + 1))
        echo "   Success: Files are identical."
    else
        echo "   Failure: Files differ."
    fi

    clear_dirs
done

echo "Killing tiny and proxy"
kill $tiny_pid 2>/dev/null
wait $tiny_pid 2>/dev/null
kill $proxy_pid 2>/dev/null
wait $proxy_pid 2>/dev/null

basicScore=$(("${MAX_BASIC}" * "${numSucceeded}" / "${numRun}"))

echo "basicScore: $basicScore/${MAX_BASIC}"

######
# Concurrency
#

echo ""
echo "*** Concurrency ***"

# Run the Tiny Web server
tiny_port=$(free_port)
echo "Starting tiny on port ${tiny_port}"
cd "./tiny" || exit
./tiny "${tiny_port}" &>/dev/null &
tiny_pid=$!
cd "${HOME_DIR}" || exit

# Wait for tiny to start in earnest
wait_for_port_use "${tiny_port}"

# Run the proxy
proxy_port=$(free_port)
echo "Starting proxy on port ${proxy_port}"
./proxy "${proxy_port}" &>/dev/null &
proxy_pid=$!

# Wait for the proxy to start in earnest
wait_for_port_use "${proxy_port}"

# Run a special blocking nop-server that never responds to requests
nop_port=$(free_port)
echo "Starting the blocking NOP server on port ${nop_port}"
./nop-server.py "${nop_port}" &>/dev/null &
nop_pid=$!

# Wait for the nop server to start in earnest
wait_for_port_use "${nop_port}"

# Try to fetch a file from the blocking nop-server using the proxy
clear_dirs
echo "Trying to fetch a file from the blocking nop-server"
download_proxy $PROXY_DIR "nop-file.txt" "http://localhost:${nop_port}/nop-file.txt" "http://localhost:${proxy_port}" &

# Fetch directly from Tiny
echo "Fetching ./tiny/${FETCH_FILE} into ${NOPROXY_DIR} directly from Tiny"
download_noproxy $NOPROXY_DIR ${FETCH_FILE} "http://localhost:${tiny_port}/${FETCH_FILE}"

# Fetch using the proxy
echo "Fetching ./tiny/${FETCH_FILE} into ${PROXY_DIR} using the proxy"
download_proxy $PROXY_DIR ${FETCH_FILE} "http://localhost:${tiny_port}/${FETCH_FILE}" "http://localhost:${proxy_port}"

# See if the proxy fetch succeeded
echo "Checking whether the proxy fetch succeeded"
if diff -q ${PROXY_DIR}/${FETCH_FILE} ${NOPROXY_DIR}/${FETCH_FILE} &>/dev/null; then
    concurrencyScore=${MAX_CONCURRENCY}
    echo "Success: Was able to fetch tiny/${FETCH_FILE} from the proxy."
else
    concurrencyScore=0
    echo "Failure: Was not able to fetch tiny/${FETCH_FILE} from the proxy."
fi

# Clean up
echo "Killing tiny, proxy, and nop-server"
kill $tiny_pid 2>/dev/null
wait $tiny_pid 2>/dev/null
kill $proxy_pid 2>/dev/null
wait $proxy_pid 2>/dev/null
kill $nop_pid 2>/dev/null
wait $nop_pid 2>/dev/null

echo "concurrencyScore: $concurrencyScore/${MAX_CONCURRENCY}"

#####
# Caching
#
echo ""
echo "*** Cache ***"

# Run the Tiny Web server
tiny_port=$(free_port)
echo "Starting tiny on port ${tiny_port}"
cd "./tiny" || exit
./tiny "${tiny_port}" &>/dev/null &
tiny_pid=$!
cd "${HOME_DIR}" || exit

# Wait for tiny to start in earnest
wait_for_port_use "${tiny_port}"

# Run the proxy
proxy_port=$(free_port)
echo "Starting proxy on port ${proxy_port}"
./proxy "${proxy_port}" &>/dev/null &
proxy_pid=$!

# Wait for the proxy to start in earnest
wait_for_port_use "${proxy_port}"

# Fetch some files from tiny using the proxy
clear_dirs
for file in ${CACHE_LIST}; do
    echo "Fetching ./tiny/${file} into ${PROXY_DIR} using the proxy"
    download_proxy $PROXY_DIR "${file}" "http://localhost:${tiny_port}/${file}" "http://localhost:${proxy_port}"
done

# Kill Tiny
echo "Killing tiny"
kill $tiny_pid 2>/dev/null
wait $tiny_pid 2>/dev/null

# Now try to fetch a cached copy of one of the fetched files.
echo "Fetching a cached copy of ./tiny/${FETCH_FILE} into ${NOPROXY_DIR}"
download_proxy $NOPROXY_DIR ${FETCH_FILE} "http://localhost:${tiny_port}/${FETCH_FILE}" "http://localhost:${proxy_port}"

# See if the proxy fetch succeeded by comparing it with the original
# file in the tiny directory
if diff -q ./tiny/${FETCH_FILE} ${NOPROXY_DIR}/${FETCH_FILE} &>/dev/null; then
    cacheScore=${MAX_CACHE}
    echo "Success: Was able to fetch tiny/${FETCH_FILE} from the cache."
else
    cacheScore=0
    echo "Failure: Was not able to fetch tiny/${FETCH_FILE} from the proxy cache."
fi

# Kill the proxy
echo "Killing proxy"
kill $proxy_pid 2>/dev/null
wait $proxy_pid 2>/dev/null

echo "cacheScore: $cacheScore/${MAX_CACHE}"

# Emit the total score
totalScore=$(("${basicScore}" + "${cacheScore}" + "${concurrencyScore}"))
maxScore=$(("${MAX_BASIC}" + "${MAX_CACHE}" + "${MAX_CONCURRENCY}"))
echo ""
echo "totalScore: ${totalScore}/${maxScore}"
exit
