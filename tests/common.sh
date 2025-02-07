#!/usr/bin/bash

#SECTION: Color Preset
CLR_BLACK="\033[30m"
CLR_RED="\033[31m"
CLR_GREEN="\033[32m"
CLR_YELLOW="\033[33m"
CLR_BLUD="\033[34m"
CLR_PURPLE="\033[35m"
CLR_BLUE="\033[36m"
CLR_GREY="\033[37m"

CLR_END="\033[0m"
#!SECTION

#SECTION: MACRO
M_DATA="DATA"
#!SECTION

#SECTION: TABLE
function table_create () {
    local TABLE_NAME
    local COLUMNS
    TABLE_NAME=$1
    COLUMNS=$2
    echo "$COLUMNS" >"$TABLE_NAME"
}

function table_add_row () {
    local TABLE_NAME
    local ROW
    TABLE_NAME=$1
    ROW=$2
    echo "$ROW" >> "$TABLE_NAME"
}
#!SECTION

#SECTION: Parse Reseult from terminal
function pmfs_attr_time_stats () {
    ATTR=$1
    TARGET_STATS=$2
    sed -i "s/\[ //g" "$TARGET_STATS"
    sed -i "s/\]//g" "$TARGET_STATS"

    CMD="awk '\$2==\"ATTR:\" {print \$6}' $TARGET_STATS"
    echo "$CMD" >/tmp/awk_nova_attr_time
    sed -i "s/ATTR/${ATTR}/g" /tmp/awk_nova_attr_time
    CMD=$(cat /tmp/awk_nova_attr_time)
    
    bash -c "$CMD" >/tmp/awk_nova_attr_time
    sed -i "s/,//g" /tmp/awk_nova_attr_time
    cat /tmp/awk_nova_attr_time
    rm /tmp/awk_nova_attr_time
}

function nova_attr_time_stats () {
    ATTR=$1
    TARGET_STATS=$2
    
    CMD="awk '\$1==\"ATTR:\" {print \$5}' $TARGET_STATS"
    echo "$CMD" >/tmp/awk_nova_attr_time
    sed -i "s/ATTR/${ATTR}/g" /tmp/awk_nova_attr_time
    CMD=$(cat /tmp/awk_nova_attr_time)
    
    bash -c "$CMD" >/tmp/awk_nova_attr_time
    sed -i "s/,//g" /tmp/awk_nova_attr_time
    cat /tmp/awk_nova_attr_time
    rm /tmp/awk_nova_attr_time
}

function nova_attr_average_stats () {
    ATTR=$1
    TARGET_STATS=$2
    
    CMD="awk '\$1==\"ATTR:\" {print \$7}' $TARGET_STATS"
    echo "$CMD" >/tmp/awk_nova_attr_time
    sed -i "s/ATTR/${ATTR}/g" /tmp/awk_nova_attr_time
    CMD=$(cat /tmp/awk_nova_attr_time)
    
    bash -c "$CMD" >/tmp/awk_nova_attr_time
    sed -i "s/,//g" /tmp/awk_nova_attr_time
    cat /tmp/awk_nova_attr_time
    rm /tmp/awk_nova_attr_time
}

function nova_attr_time () {
    ATTR=$1
    TARGET_DMESG=$2
    
    CMD="awk '\$3==\"ATTR:\" {print \$7}' $TARGET_DMESG"
    echo "$CMD" >/tmp/awk_nova_attr_time
    sed -i "s/ATTR/${ATTR}/g" /tmp/awk_nova_attr_time
    CMD=$(cat /tmp/awk_nova_attr_time)
    
    bash -c "$CMD" >/tmp/awk_nova_attr_time
    sed -i "s/,//g" /tmp/awk_nova_attr_time
    cat /tmp/awk_nova_attr_time
    rm /tmp/awk_nova_attr_time
}

function pmfs_attr_time () {
    ATTR=$1
    TARGET_DMESG=$2
    
    CMD="awk '\$2==\"ATTR:\" {print \$6}' $TARGET_DMESG"
    echo "$CMD" >/tmp/awk_pmfs_attr_time
    sed -i "s/ATTR/${ATTR}/g" /tmp/awk_pmfs_attr_time
    CMD=$(cat /tmp/awk_pmfs_attr_time)
    
    bash -c "$CMD" >/tmp/awk_pmfs_attr_time
    sed -i "s/,//g" /tmp/awk_pmfs_attr_time
    cat /tmp/awk_pmfs_attr_time
    rm /tmp/awk_pmfs_attr_time
}

function splitfs_attr_time() {
    ATTR=$1
    TARGET_MESG=$2
    WL=$3

    if [[ "${WL}" == *"FILEBENCH"* ]]; then
        line=$(grep -n "Run took 60 seconds" "$TARGET_MESG" | cut -d: -f1)
        awk -v line="$line" 'NR > line' "$TARGET_MESG" | grep "$ATTR: timing = " | awk '{print $6}' | head -n 1
    else
        # select the first
        cat "$TARGET_MESG" | grep "$ATTR: timing = " | awk '{print $6}' | head -n 1
    fi
    
}

function nova_attr_average () {
    ATTR=$1
    TARGET_DMESG=$2
    
    CMD="awk '\$3==\"ATTR:\" {print \$9}' $TARGET_DMESG"
    echo "$CMD" >/tmp/awk_nova_attr_time
    sed -i "s/ATTR/${ATTR}/g" /tmp/awk_nova_attr_time
    CMD=$(cat /tmp/awk_nova_attr_time)
    
    bash -c "$CMD" >/tmp/awk_nova_attr_time
    sed -i "s/,//g" /tmp/awk_nova_attr_time
    cat /tmp/awk_nova_attr_time
    rm /tmp/awk_nova_attr_time
}

function filebench_attr_iops() {
    FILE_BENCH_OUT=$1
    awk '$7=="ops/s" {print $6}' "$FILE_BENCH_OUT"
}

function filebench_attr_breakdown() {
    FILE_BENCH_OUT=$1
    ATTR_BD=$2
    a=$(cat "$FILE_BENCH_OUT" | grep -w "$ATTR_BD" | awk '{print $5}' | sed 's#ms/op##g')
    echo "$a"
}
#!SECTION

#SECTION: Arithmethic
function log () {
    python3 -c "import math; print(int(math.log2($1)))"
}

#!SECTION


#SECTION: GET Functions
function where_is_script() {
    local script=$1
    cd "$( dirname "$script" )" && pwd
}
#!SECTION


#SECTION: COMMON functions
function get_pmem_block () {
    local PMEM=$1
    df --block-size=1024K /dev/"$PMEM" >/tmp/get_pmem_block
    awk 'NR!=1 {print $3}' /tmp/get_pmem_block
    rm /tmp/get_pmem_block
}

function absolute_path() {
    local path=$1
    echo "$(cd "$path" || exit; pwd)"
}

function split_workset() {
    local WORK_SET_SIZE
    local NUM_JOBS
    local PER_JOB_SIZE

    WORK_SET_SIZE=$1
    NUM_JOBS=$2

    echo -e "Splitting Workset ${WORK_SET_SIZE}MB for ${NUM_JOBS} jobs." 1>&2
    
    PER_JOB_SIZE=$((WORK_SET_SIZE / NUM_JOBS))

    echo -e "$CLR_GREEN""${PER_JOB_SIZE}MB for per job.""$CLR_END" 1>&2
    
    echo $PER_JOB_SIZE
}

function run_test () {
    local TEST
    local TARGET
    local OUTPUT
    
    TEST=$1
    TARGET=$2
    OUTPUT=$3

    echo "TEST:     $TEST"
    echo "TARGET:   $TARGET"
    echo "OUTPUT:   $OUTPUT"
    echo "==============================="
    echo -e "$CLR_GREEN""Running...""$CLR_END"
    $TEST > "$OUTPUT"
    echo -e "$CLR_GREEN""END""$CLR_END"
    echo "==============================="
}

#!SECTION