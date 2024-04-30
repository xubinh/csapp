#!/usr/bin/env bash

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

# 运行总测试 (提供概览):
echo '--------------------------------'
./mdriver -t traces/ -V
echo ""
echo ""

# 单独运行每个测试 (提供详细信息):
for trace_file in "${trace_files[@]}"; do
    echo '--------------------------------'
    ./mdriver -f traces/"$trace_file" -V -l
    echo ""
    echo ""
done
