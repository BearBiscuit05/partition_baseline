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

# FILEPATH='/home/bear/workspace/single-gnn/data/raid/reddit/output.txt'
# DATANAME='RD'

# FILEPATH='/home/bear/TrillionG/3B_output/graph.txt'

# IFSAVE=false
# SAVENAME="random_${DATANAME}_${PARTITION}"


monitor_memory() {
    local python_pid="$1"
    local mem_log="$2"
    echo "=============================================" >> $mem_log
    

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
    echo "=============================================" >> $mem_log
}




# FILEPATH='/home/bear/TrillionG/10B_output/graph.txt'
# PARTITION=64
# IFSAVE=false
# SAVENAME="dbh_${DATANAME}_${PARTITION}"

p_values=(64)
FILEPATH='/home/bear/TrillionG/0.57_output/graph.txt'
memory_usage_file="../script/greedy_mem_11_29.log"
interval=10
for p in "${p_values[@]}"; do
    ./greedy -filename ${FILEPATH} -p ${p} -shuffle false &
    lastPid=$!
    logMsg="./greedy -filename ${FILEPATH} -p ${p} -shuffle false"
    echo $logMsg >> $memory_usage_file
    monitor_memory $lastPid $memory_usage_file $interval
done


FILEPATH='/home/bear/TrillionG/0.63_output/graph.txt'
p_values=(64)
memory_usage_file="../script/greedy_mem_11_29.log"
interval=10
for p in "${p_values[@]}"; do
    ./greedy -filename ${FILEPATH} -p ${p} -shuffle false &
    lastPid=$!
    logMsg="./greedy -filename ${FILEPATH} -p ${p} -shuffle false"
    echo $logMsg >> $memory_usage_file
    monitor_memory $lastPid $memory_usage_file $interval
done









