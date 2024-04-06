#!/bin/bash

memory_usage_file="/home/bear/workspace/partition_baseline/log/memory_usage.txt"
interval=1 
monitor_memory() {
    echo "=============================================" >> $memory_usage_file
    local python_pid="$1"

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
    echo "平均内存占用: $average_memory_usage_mb MB" >> $memory_usage_file
    echo "峰值内存占用: $((peak_memory_usage_kb / 1024)) MB" >> $memory_usage_file
    echo "=============================================" >> $memory_usage_file
}


cd ../build/

# 定义文件路径和分区数的关联数组
declare -A FILE_PARTITION=(
    # ['/raid/bear/tmp/RD_graph.txt']=4
    # ['/raid/bear/tmp/PD_graph.txt']=8
    # ['/raid/bear/tmp/PA_graph.txt']=4
    # ['/raid/bear/tmp/FR_graph.txt']=8
    # ['/raid/bear/tmp/WB_graph.txt']=8
     ['/raid/bear/tmp/UK_graph.txt']=4
)
# 设置其他变量
IFSAVE=true
SAVENAME_BASE="/raid/bear/tmp/random_partition/"

# 循环遍历关联数组并执行命令
for FILEPATH in "${!FILE_PARTITION[@]}"; do
    PARTITION=${FILE_PARTITION[$FILEPATH]}
    SAVENAME="${SAVENAME_BASE}$(basename ${FILEPATH%.*})"

    echo "Processing file: ${FILEPATH}"
    echo "Partition: ${PARTITION}"
    echo "Save name: ${SAVENAME}"

    # # 执行命令
    ./random -filename ${FILEPATH} -p ${PARTITION} -shuffle false -write_parts true -parts_filename ${SAVENAME} &
    
    lastPid=$!
    monitor_memory $lastPid
done


