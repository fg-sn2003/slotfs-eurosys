#!/usr/bin/bash

function where_is_script() {
    local script=$1
    cd "$(dirname "$script")" && pwd
}

ABS_PATH=$(where_is_script "$0")

NOVA_PATH="$ABS_PATH"/../nova
PMFS_PATH="$ABS_PATH"/../pmfs
SPLITFS_PATH="$ABS_PATH"/../splitfs/splitfs
EXT4RELINK_PATH="$ABS_PATH"/../ext4relink
MADFS_PATH="$ABS_PATH"/../madfs
SLOTFS_PATH="$ABS_PATH"/../slotfs
CONFIGS_PATH="$ABS_PATH"/configs

function compile_splitfs() {
    local measure_timing=$1
    make clean
    
    if ((measure_timing == 1)); then
        export LEDGER_INSTRU=1    
        # export LEDGER_DEBUG=1
        # export LIBNVP_DEBUG=1
    fi
    
    make -e -j"$(nproc)"

    sudo umount /mnt/pmem0
    # sudo rmmod ext4relink

    # cd "$EXT4RELINK_PATH" || exit
    # make -j32
    # sudo insmod ext4relink.ko
    # cd - || exit

    sudo mkfs.ext4 -F -b 4096 /dev/pmem0
    sudo mount -o dax /dev/pmem0 /mnt/pmem0
}

function compile_madfs() {    
    make BUILD_TARGETS="madfs"
    sudo umount /mnt/pmem0
    sudo mkfs.ext4 -F -b 4096 /dev/pmem0
    sudo mount -o dax /dev/pmem0 /mnt/pmem0
}

if [ ! $1 ] || [ ! $2 ]; then
    echo "Usage: $0 <fs> <branch>"
    exit 1
fi

fs=$1
branch=$2

if [ ! "$3" ]; then
    measure_timing=0
else
    measure_timing=$3
fi

# free memory at first
echo "Drop caches..."
sync
echo 1 | sudo tee /proc/sys/vm/drop_caches
# sync
# echo 1 | sudo tee /proc/sys/vm/drop_caches
# sync
# echo 1 | sudo tee /proc/sys/vm/drop_caches

# setup
case "${fs}" in
"NOVA")
    cd "$NOVA_PATH" || exit
    git checkout "$branch"
    if ((measure_timing == 1)); then
        bash setup.sh "$CONFIGS_PATH"/nova/config.mt.json
    else
        bash setup.sh
    fi
    ;;
"NOVA-nogc")
    cd "$NOVA_PATH" || exit
    git checkout "$branch"
    bash setup.sh "$CONFIGS_PATH"/nova/config.nogc.json
    ;;
"NOVA-fragment")
    cd "$NOVA_PATH" || exit
    git checkout "$branch"
    bash setup.sh "$CONFIGS_PATH"/nova/config.fragment.json
    ;;
"PMFS")
    cd "$PMFS_PATH" || exit
    git checkout "$branch"
    bash setup.sh /dev/pmem0 /mnt/pmem0 -j32 "$measure_timing"
    ;;
"EXT4-DAX")
    sudo umount /mnt/pmem0
    sudo mkfs.ext4 -F -b 4096 /dev/pmem0
    sudo mount -t ext4 -o dax /dev/pmem0 /mnt/pmem0
    ;;
"XFS-DAX")
    sudo umount /mnt/pmem0
    sudo mkfs -t xfs -m reflink=0 -f /dev/pmem0
    sudo mount -t xfs -o dax /dev/pmem0 /mnt/pmem0
    ;;
"SplitFS-FIO")
    cd "$SPLITFS_PATH" || exit
    export LEDGER_DATAJ=0
    export LEDGER_POSIX=1
    export LEDGER_FIO=1
    compile_splitfs "$measure_timing"
    ;;
"SplitFS-FIO-STRICT")
    cd "$SPLITFS_PATH" || exit
    export LEDGER_DATAJ=1
    export LEDGER_POSIX=0
    export LEDGER_FIO=1
    compile_splitfs
    ;;
"SplitFS-FILEBENCH")
    cd "$SPLITFS_PATH" || exit
    export LEDGER_DATAJ=0
    export LEDGER_POSIX=0
    export LEDGER_FILEBENCH=1
    compile_splitfs "$measure_timing"
    ;;
"SplitFS-FILEBENCH-STRICT")
    cd "$SPLITFS_PATH" || exit
    export LEDGER_DATAJ=1
    export LEDGER_POSIX=0
    export LEDGER_FILEBENCH=1
    compile_splitfs "$measure_timing"
    ;;
"SplitFS-YCSB")
    cd "$SPLITFS_PATH" || exit
    export LEDGER_DATAJ=0
    export LEDGER_POSIX=1
    export LEDGER_YCSB=1
    compile_splitfs
    ;;
"SplitFS-YCSB-STRICT")
    cd "$SPLITFS_PATH" || exit
    export LEDGER_DATAJ=1
    export LEDGER_POSIX=0
    export LEDGER_YCSB=1
    compile_splitfs
    ;;
"SplitFS-TPCC")
    cd "$SPLITFS_PATH" || exit
    export LEDGER_DATAJ=0
    export LEDGER_POSIX=1
    export LEDGER_TPCC=1
    compile_splitfs
    ;;
"SplitFS")
    cd "$SPLITFS_PATH" || exit
    export LEDGER_DATAJ=0
    export LEDGER_POSIX=1
    compile_splitfs
    ;;

"MadFS")
    cd "$MADFS_PATH" || exit
    make BUILD_TARGETS="madfs"
    sudo umount /mnt/pmem0
    sudo mkfs.ext4 -F -b 4096 /dev/pmem0
    sudo mount -o dax /dev/pmem0 /mnt/pmem0
    ;;
"SLOTFS")
    cd "$SLOTFS_PATH" || exit
    bash setup.sh 
    ;;
*)
    echo "Unknown file system: $fs"
    exit 1
    ;;
esac
