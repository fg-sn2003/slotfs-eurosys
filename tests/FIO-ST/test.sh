#!/usr/bin/bash
# shellcheck source=/dev/null

source "../common.sh"
ABS_PATH=$(where_is_script "$0")
TOOLS_PATH=$ABS_PATH/../../tools
FSCRIPT_PRE_FIX=$ABS_PATH/../../tools/fbscripts
BOOST_DIR=$ABS_PATH/../../splitfs/splitfs
SLOTFS_DIR=$ABS_PATH/../../slotfs
MADFS_DIR=$ABS_PATH/../../madfs

FILE_SIZES=($((1 * 1024)))
FILE_SYSTEMS=("SLOTFS" "MadFS" "NOVA" "PMFS" "SplitFS-FIO" "EXT4-DAX")
# FILE_SYSTEMS=( "SLOTFS")
# BLK_SIZES=(512B $((1 * 1024))B  $((2 * 1024))B $((4 * 1024))B $((8 * 1024))B $((16 * 1024))B $((32 * 1024))B)
BLK_SIZES=($((1 * 1024))B  $((4 * 1024))B $((16 * 1024))B)

# FILE_SYSTEMS=("SLOTFS")
# BLK_SIZES=($((1 * 1024))B  $((4 * 1024))B  $((16 * 1024))B)

TABLE_NAME="$ABS_PATH/performance-table"
table_create "$TABLE_NAME" "ops file_system file_size blk_size bandwidth(MiB/s)"

loop=1
if [ "$1" ]; then
    loop=$1
fi

for ((i = 1; i <= loop; i++)); do
    for file_system in "${FILE_SYSTEMS[@]}"; do
        for fsize in "${FILE_SIZES[@]}"; do
            for bsize in "${BLK_SIZES[@]}"; do
                if [[ "${file_system}" == "SLOTFS" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export SLOTFS_MKFS=1
                    BW=$(LD_PRELOAD=$SLOTFS_DIR/build/libslot.so fio -filename="\\test" --overwrite=1 --fsync=1 -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                elif [[ "${file_system}" =~ "SplitFS-FIO" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH="$BOOST_DIR"
                    export NVP_TREE_FILE="$BOOST_DIR"/bin/nvp_nvp.tree
                    BW=$(LD_PRELOAD=$BOOST_DIR/libnvp.so fio -filename="/mnt/pmem0/test" --overwrite=1 -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                elif [[ "${file_system}" == "MadFS" ]]; then
                    rm -rf /dev/shm/*
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH
                    BW=$(LD_PRELOAD=$MADFS_DIR/build-release/libmadfs.so fio -filename="/mnt/pmem0/test" --overwrite=1 --fsync=1 -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                else
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "main" "0"
                    BW=$(fio -filename="/mnt/pmem0/test" --overwrite=1 -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                fi
                table_add_row "$TABLE_NAME" "seq-overwrite $file_system $fsize $bsize $BW"


                if [[ "${file_system}" == "SLOTFS" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export SLOTFS_MKFS=1
                    BW=$(LD_PRELOAD=$SLOTFS_DIR/build/libslot.so fio -filename="\\test" -fallocate=none --fsync=1 --overwrite=1 -direct=0 -iodepth 1 -rw=randwrite -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                elif [[ "${file_system}" =~ "SplitFS-FIO" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH="$BOOST_DIR"
                    export NVP_TREE_FILE="$BOOST_DIR"/bin/nvp_nvp.tree
                    BW=$(LD_PRELOAD=$BOOST_DIR/libnvp.so fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 --overwrite=1 -iodepth 1 -rw=randwrite -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                elif [[ "${file_system}" == "MadFS" ]]; then
                    rm -rf /dev/shm/*
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH
                    BW=$(LD_PRELOAD=$MADFS_DIR/build-release/libmadfs.so fio -filename="/mnt/pmem0/test" -fallocate=none --fsync=1 --overwrite=1 -direct=0 -iodepth 1 -rw=randwrite -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                else
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "main" "0"
                    BW=$(fio -filename="/mnt/pmem0/test" -fallocate=none --fsync=1 --overwrite=1 -direct=0 -iodepth 1 -rw=randwrite -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                fi
                table_add_row "$TABLE_NAME" "rnd-overwrite $file_system $fsize $bsize $BW"

                if [[ "${file_system}" == "SLOTFS" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export SLOTFS_MKFS=1
                    BW=$(LD_PRELOAD=$SLOTFS_DIR/build/libslot.so fio -filename="\\test" -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                elif [[ "${file_system}" =~ "SplitFS-FIO" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH="$BOOST_DIR"
                    export NVP_TREE_FILE="$BOOST_DIR"/bin/nvp_nvp.tree
                    BW=$(LD_PRELOAD=$BOOST_DIR/libnvp.so fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                elif [[ "${file_system}" == "MadFS" ]]; then
                    rm -rf /dev/shm/*
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH
                    BW=$(LD_PRELOAD=$MADFS_DIR/build-release/libmadfs.so fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                else
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "main" "0"
                    BW=$(fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                fi
                table_add_row "$TABLE_NAME" "seq-write $file_system $fsize $bsize $BW"


                if [[ "${file_system}" == "SLOTFS" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export SLOTFS_MKFS=1
                    BW=$(LD_PRELOAD=$SLOTFS_DIR/build/libslot.so fio -filename="\\test" -fallocate=none -direct=0 -iodepth 1 -rw=randwrite -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                elif [[ "${file_system}" =~ "SplitFS-FIO" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH="$BOOST_DIR"
                    export NVP_TREE_FILE="$BOOST_DIR"/bin/nvp_nvp.tree
                    BW=$(LD_PRELOAD=$BOOST_DIR/libnvp.so fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=randwrite -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                elif [[ "${file_system}" == "MadFS" ]]; then
                    rm -rf /dev/shm/*
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH
                    BW=$(LD_PRELOAD=$MADFS_DIR/build-release/libmadfs.so fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=randwrite -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                else
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "main" "0"
                    BW=$(fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=randwrite -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                fi
                table_add_row "$TABLE_NAME" "rnd-write $file_system $fsize $bsize $BW"

                if [[ "${file_system}" == "SLOTFS" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export SLOTFS_MKFS=1
                    BW=$(LD_PRELOAD=$SLOTFS_DIR/build/libslot.so fio -filename="\\test" -fallocate=none -direct=0 -iodepth 1 -rw=read -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep READ: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                elif [[ "${file_system}" =~ "SplitFS-FIO" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH="$BOOST_DIR"
                    export NVP_TREE_FILE="$BOOST_DIR"/bin/nvp_nvp.tree
                    BW=$(LD_PRELOAD=$BOOST_DIR/libnvp.so fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=read -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep READ: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                elif [[ "${file_system}" == "MadFS" ]]; then
                    rm -rf /dev/shm/*
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH
                    BW=$(LD_PRELOAD=$MADFS_DIR/build-release/libmadfs.so fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=read -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep READ: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                else
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "main" "0"
                    BW=$(fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=read -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep READ: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                fi
                table_add_row "$TABLE_NAME" "seq-read $file_system $fsize $bsize $BW"

                if [[ "${file_system}" == "SLOTFS" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export SLOTFS_MKFS=1
                    BW=$(LD_PRELOAD=$SLOTFS_DIR/build/libslot.so  fio -filename="\\test" -fallocate=none -direct=0 -iodepth 1 -rw=randread -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep READ: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                elif [[ "${file_system}" =~ "SplitFS-FIO" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH="$BOOST_DIR"
                    export NVP_TREE_FILE="$BOOST_DIR"/bin/nvp_nvp.tree
                    BW=$(LD_PRELOAD=$BOOST_DIR/libnvp.so fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=randread -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep READ: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                elif [[ "${file_system}" == "MadFS" ]]; then
                    rm -rf /dev/shm/*
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH
                    BW=$(LD_PRELOAD=$MADFS_DIR/build-release/libmadfs.so fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=randread -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep READ: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                else
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "main" "0"
                    BW=$(fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=randread -ioengine=sync -bs="$bsize" -size="$fsize"M -name=test | grep READ: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                fi
                table_add_row "$TABLE_NAME" "rnd-read $file_system $fsize $bsize $BW"
            done
        done
    done
done

