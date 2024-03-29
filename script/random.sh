#!/bin/bash

cd ../build/
# FILEPATH='/home/bear/workspace/single-gnn/data/partition/TW/output.txt'
# DATANAME='TW'

# FILEPATH='/home/bear/workspace/single-gnn/data/partition/FR/output.txt'
# DATANAME='FR'

# FILEPATH='/home/bear/workspace/single-gnn/data/partition/UK/output.txt'
# DATANAME='UK'

# FILEPATH='/home/bear/workspace/single-gnn/data/partition/PA/output.txt'
# DATANAME='PA'

FILEPATH='/home/bear/workspace/single-gnn/data/raid/reddit/output.txt'
DATANAME='RD'

# FILEPATH='/home/bear/workspace/single-gnn/data/raid/reddit/output.txt'
# DATANAME='RD'

FILEPATH='/home/bear/TrillionG/10B_output/graph.bin'
PARTITION=4
# IFSAVE=false
# SAVENAME="random_${DATANAME}_${PARTITION}"
interval=3
mem_log=""

monitor_memory() {
    echo "=============================================" >> $mem_log
    local python_pid="$1"
    local mem_log="$2"

    local total_memory_usage_kb=0
    local peak_memory_usage_kb=0
    local count=0
    while ps -p $python_pid > /dev/null; do
        local memory_usage_kb=$(ps -o rss= -p $python_pid)
        local memory_usage_mb=$(echo "scale=2; $memory_usage_kb / 1024" | bc)
        local current_time=$(date +'%Y-%m-%d %H:%M:%S')
        total_memory_usage_kb=$((total_memory_usage_kb + memory_usage_kb))
        if [ $memory_usage_kb -gt $peak_memory_usage_kb ]; then
            peak_memory_usage_kb=$memory_usage_kb
        fi
        count=$((count + 1))
        sleep $interval
    done
    if [ $count -gt 0 ]; then
        local average_memory_usage_kb=$((total_memory_usage_kb / count))
        local average_memory_usage_mb=$(echo "scale=2; $average_memory_usage_kb / 1024" | bc)
    else
        local average_memory_usage_mb=0
    fi
    echo "平均内存占用: $average_memory_usage_mb MB" >> $mem_log
    echo "峰值内存占用: $((peak_memory_usage_kb / 1024)) MB" >> $mem_log
}




./random -filename ${FILEPATH} -p ${PARTITION} -shuffle false