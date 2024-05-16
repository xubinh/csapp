#!/usr/bin/env bash

TIMEOUT=5
HOME_DIR="$(pwd)"
PROXY_DIR="./.proxy"
NOPROXY_DIR="./.noproxy"
FILE=home.html
BASIC_LIST="home.html
            csapp.c
            tiny.c
            godzilla.jpg
            tiny"
BASIC_LIST_2="tiny
            tiny2
            tiny3
            tiny4
            tiny5
            tiny6"
tiny_port="15214"
proxy_port="15213"

function download_proxy {
    cd "$1" || exit
    # curl -v --max-time ${TIMEOUT} --silent --proxy "$4" --output "$2" "$3"
    curl --max-time ${TIMEOUT} --silent --proxy "$4" --output "$2" "$3"
    (($? == 28)) && echo "Error: Fetch timed out after ${TIMEOUT} seconds"
    cd "$HOME_DIR" || exit
}

function download_noproxy {
    cd "$1" || exit
    curl -v --max-time ${TIMEOUT} --silent --output "$2" "$3"
    # curl --max-time ${TIMEOUT} --silent --output "$2" "$3"
    (($? == 28)) && echo "Error: Fetch timed out after ${TIMEOUT} seconds"
    cd "$HOME_DIR" || exit
}

function clear_dirs {
    rm -rf "${PROXY_DIR:?}"/*
    rm -rf "${NOPROXY_DIR:?}"/*
}

# shellcheck disable=SC2120
function initialize() {
    local verbose_option=''

    # 检测参数中是否有 verbose 选项:
    while (("$#")); do
        case "$1" in
        -v | --verbose)
            verbose_option='true'
            break
            ;;
        esac

        shift
    done

    make clean

    if [ ! -d ${PROXY_DIR} ]; then
        mkdir ${PROXY_DIR}
    fi

    if [ ! -d ${NOPROXY_DIR} ]; then
        mkdir ${NOPROXY_DIR}
    fi

    make

    if [ ! -x ./tiny/tiny ]; then
        (
            cd "./tiny" || exit
            make
        )
    fi

    cd ./tiny || exit
    if [ "$verbose_option" == 'true' ]; then
        ./tiny "$tiny_port" &
    else
        ./tiny "$tiny_port" &>/dev/null &
    fi
    cd ..

    if [ "$verbose_option" == 'true' ]; then
        ./proxy "$proxy_port" &
    else
        ./proxy "$proxy_port" &>/dev/null &
    fi
}

function finish() {
    ./kill_process.sh
}

function test_naive() {
    initialize

    # telnet localhost "$proxy_port" <test_http_request.txt | tee response.txt  # 这行命令无法产生想要的效果
    telnet localhost "$proxy_port" | tee response.txt

    finish
}

function test_naive_2() {
    initialize

    curl --proxy http://localhost:"$proxy_port" --output ./.proxy/home.html http://localhost:"$tiny_port"/home.html
    curl --output ./.noproxy/home.html http://localhost:"$tiny_port"/home.html

    finish
}

function test_naive_3() {
    initialize

    download_proxy "$PROXY_DIR" "${FILE}" "http://localhost:$tiny_port/${FILE}" "http://localhost:$proxy_port"
    download_noproxy "$NOPROXY_DIR" "${FILE}" "http://localhost:$tiny_port/${FILE}"

    finish
}

function test_basic() {
    initialize

    numRun=0
    numSucceeded=0

    for file in ${BASIC_LIST}; do
        numRun=$(("$numRun" + 1))
        echo "${numRun}: ${file}"

        echo "   Fetching ./tiny/${file} into ${PROXY_DIR} using the proxy"
        download_proxy $PROXY_DIR "${file}" "http://localhost:${tiny_port}/${file}" "http://localhost:${proxy_port}"

        echo "   Fetching ./tiny/${file} into ${NOPROXY_DIR} directly from Tiny"
        download_noproxy $NOPROXY_DIR "${file}" "http://localhost:${tiny_port}/${file}"

        echo "   Comparing the two files"
        if diff -q "${PROXY_DIR}/${file}" "${NOPROXY_DIR}/${file}" &>/dev/null; then
            numSucceeded=$(("${numSucceeded}" + 1))
            echo "   Success: Files are identical."
        else
            echo "   Failure: Files differ."
        fi

        clear_dirs
    done

    echo "numSucceeded: $numSucceeded"

    finish
}

function test_basic_2() {
    initialize -v

    numRun=0
    numSucceeded=0

    for file in ${BASIC_LIST_2}; do
        numRun=$(("$numRun" + 1))
        echo "${numRun}: ${file}"

        echo "   Fetching ./tiny/${file} into ${PROXY_DIR} using the proxy"
        download_proxy $PROXY_DIR "${file}" "http://localhost:${tiny_port}/${file}" "http://localhost:${proxy_port}"

        echo "   Fetching ./tiny/${file} into ${NOPROXY_DIR} directly from Tiny"
        download_noproxy $NOPROXY_DIR "${file}" "http://localhost:${tiny_port}/${file}"

        echo "   Comparing the two files"
        if diff -q "${PROXY_DIR}/${file}" "${NOPROXY_DIR}/${file}" &>/dev/null; then
            numSucceeded=$(("${numSucceeded}" + 1))
            echo "   Success: Files are identical."
        else
            echo "   Failure: Files differ."
        fi

        clear_dirs
    done

    echo "numSucceeded: $numSucceeded"

    finish
}

function test_basic_3() {
    initialize -v

    numRun=0

    for file in ${BASIC_LIST_2}; do
        numRun=$(("$numRun" + 1))
        echo "${numRun}: ${file}"

        echo "   Fetching ./tiny/${file} into ${PROXY_DIR} using the proxy"

        cd "$PROXY_DIR" || exit

        curl --max-time ${TIMEOUT} --silent --proxy "http://localhost:${proxy_port}" --output "${file}" "http://localhost:${tiny_port}/${file}"

        (($? == 28)) && echo "Error: Fetch timed out after ${TIMEOUT} seconds"

        cd "$HOME_DIR" || exit

        clear_dirs
    done

    finish
}

function test_cache() {
    initialize -v

    download_proxy "$PROXY_DIR" "${FILE}" "http://localhost:$tiny_port/${FILE}" "http://localhost:$proxy_port"

    ./kill_processes_by_keyword.sh "./tiny"

    download_proxy "$PROXY_DIR" "${FILE}" "http://localhost:$tiny_port/${FILE}" "http://localhost:$proxy_port"

    finish
}

test_basic_3
