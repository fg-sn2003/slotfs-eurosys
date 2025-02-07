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
FILE_SYSTEMS=("SLOTFS" "MADFS" "NOVA" "PMFS" "SplitFS-FIO" "EXT4-DAX")
NUM_JOBS=( 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16)

TABLE_NAME="$ABS_PATH/performance-comparison-table-SplitFS2z"
table_create "$TABLE_NAME" "file_system ops job file_size bandwidth(MiB/s)"

loop=1
if [ "$1" ]; then
    loop=$1
fi

mkdir \\/   #for slotfs

for ((i=1; i <= loop; i++))
do
    for file_system in "${FILE_SYSTEMS[@]}"; do
        for fsize in "${FILE_SIZES[@]}"; do
            for job in "${NUM_JOBS[@]}"; do
                if (( job == 1 )); then
                    fpath="/mnt/pmem0/test"
                else 
                    fpath="/mnt/pmem0/"
                fi

                # seq-overwrite
                if [[ "${file_system}" == "SLOTFS" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export SLOTFS_MKFS=1
                    if (( job == 1 )); then
                        BW=$(LD_PRELOAD=$SLOTFS_DIR/build/libslot.so fio -filename="\\test" -fallocate=none -direct=0 -iodepth 1 -rw=write --overwrite=1 -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    else
                        BW=$(LD_PRELOAD=$SLOTFS_DIR/build/libslot.so fio -directory="\\slotfs_testdir" -fallocate=none -direct=0 -iodepth 1 -thread -numjobs="$job" --overwrite=1 -rw=write -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    fi
                elif [[ "${file_system}" == "SplitFS-FIO" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH="$BOOST_DIR"
                    export NVP_TREE_FILE="$BOOST_DIR"/bin/nvp_nvp.tree
                    if (( job == 1 )); then
                        BW=$(LD_PRELOAD=$BOOST_DIR/libnvp.so fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=write --overwrite=1 -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    else
                        BW=$(LD_PRELOAD=$BOOST_DIR/libnvp.so fio -directory=/mnt/pmem0 -fallocate=none -direct=0 -iodepth 1 -thread -numjobs="$job" --overwrite=1 -rw=write -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    fi
                elif [[ "${file_system}" == "MadFS" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH
                    if (( job == 1 )); then
                        BW=$(LD_PRELOAD=$MADFS_DIR/build-release/libmadfs.so fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=write --overwrite=1 -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    else
                        BW=$(LD_PRELOAD=$MADFS_DIR/build-release/libmadfs.so fio -directory=/mnt/pmem0 -fallocate=none -direct=0 -iodepth 1 -thread -numjobs="$job" --overwrite=1 -rw=write -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    fi
                else
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "main" "0"
                    if (( job == 1 )); then
                        BW=$(fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=write --overwrite=1 -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    else
                        BW=$(fio -directory=/mnt/pmem0 -fallocate=none -direct=0 -iodepth 1 -thread -numjobs="$job" --overwrite=1 -rw=write -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    fi

                fi
                table_add_row "$TABLE_NAME" "$file_system seq-overwrite $job $fsize $BW"


                #rand-overwrite
                if [[ "${file_system}" == "SLOTFS" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export SLOTFS_MKFS=1
                    if (( job == 1 )); then
                        BW=$(LD_PRELOAD=$SLOTFS_DIR/build/libslot.so fio -filename="\\test" -fallocate=none -direct=0 -iodepth 1 -rw=randwrite --overwrite=1 -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    else
                        BW=$(LD_PRELOAD=$SLOTFS_DIR/build/libslot.so fio -directory="\\/" -fallocate=none -direct=0 -iodepth 1 -thread -numjobs="$job" --overwrite=1 -rw=randwrite -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    fi
                elif [[ "${file_system}" == "SplitFS-FIO" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH="$BOOST_DIR"
                    export NVP_TREE_FILE="$BOOST_DIR"/bin/nvp_nvp.tree
                    if (( job == 1 )); then
                        BW=$(LD_PRELOAD=$BOOST_DIR/libnvp.so fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=randwrite --overwrite=1 -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    else
                        BW=$(LD_PRELOAD=$BOOST_DIR/libnvp.so fio -directory=/mnt/pmem0 -fallocate=none -direct=0 -iodepth 1 -thread -numjobs="$job" -rw=randwrite --overwrite=1 -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    fi
                elif [[ "${file_system}" == "MadFS" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH
                    if (( job == 1 )); then
                        BW=$(LD_PRELOAD=$MADFS_DIR/build-release/libmadfs.so fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=randwrite --overwrite=1 -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    else
                        BW=$(LD_PRELOAD=$MADFS_DIR/build-release/libmadfs.so fio -directory=/mnt/pmem0 -fallocate=none -direct=0 -iodepth 1 -thread -numjobs="$job" --overwrite=1 -rw=randwrite -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    fi
                else
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "main" "0"
                    if (( job == 1 )); then
                        BW=$(fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=randwrite --overwrite=1 -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    else
                        BW=$(fio -directory=/mnt/pmem0 -fallocate=none -direct=0 -iodepth 1 -thread -numjobs="$job" --overwrite=1 -rw=randwrite -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    fi

                fi
                table_add_row "$TABLE_NAME" "$file_system rnd-overwrite $job $fsize $BW"


                # seq-write
                if [[ "${file_system}" == "SLOTFS" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export SLOTFS_MKFS=1
                    if (( job == 1 )); then
                        BW=$(LD_PRELOAD=$SLOTFS_DIR/build/libslot.so fio -filename="\\test" -fallocate=none -direct=0 -iodepth 1 -rw=write  -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    else
                        BW=$(LD_PRELOAD=$SLOTFS_DIR/build/libslot.so fio -directory="\\/" -fallocate=none -direct=0 -iodepth 1 -thread -numjobs="$job"  -rw=write -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    fi
                elif [[ "${file_system}" == "SplitFS-FIO" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH="$BOOST_DIR"
                    export NVP_TREE_FILE="$BOOST_DIR"/bin/nvp_nvp.tree
                    if (( job == 1 )); then
                        BW=$(LD_PRELOAD=$BOOST_DIR/libnvp.so fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    else
                        BW=$(LD_PRELOAD=$BOOST_DIR/libnvp.so fio -directory=/mnt/pmem0 -fallocate=none -direct=0 -iodepth 1 -thread -numjobs="$job" -rw=write -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    fi
                elif [[ "${file_system}" == "MadFS" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH
                    if (( job == 1 )); then
                        BW=$(LD_PRELOAD=$MADFS_DIR/build-release/libmadfs.so fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    else
                        BW=$(LD_PRELOAD=$MADFS_DIR/build-release/libmadfs.so fio -directory=/mnt/pmem0 -fallocate=none -direct=0 -iodepth 1 -thread -numjobs="$job" -rw=write -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    fi
                else
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "main" "0"
                    BW=$(bash "$TOOLS_PATH"/fio.sh "$fpath" 4K "$fsize" "$job" | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                fi
                table_add_row "$TABLE_NAME" "$file_system seq-write $job $fsize $BW"


                # rand-write
                if [[ "${file_system}" == "SLOTFS" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export SLOTFS_MKFS=1
                    if (( job == 1 )); then
                        BW=$(LD_PRELOAD=$SLOTFS_DIR/build/libslot.so fio -filename="\\test" -fallocate=none -direct=0 -iodepth 1 -rw=randwrite  -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    else
                        BW=$(LD_PRELOAD=$SLOTFS_DIR/build/libslot.so fio -directory="\\/" -fallocate=none -direct=0 -iodepth 1 -thread -numjobs="$job"  -rw=randwrite -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    fi
                elif [[ "${file_system}" == "SplitFS-FIO" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH="$BOOST_DIR"
                    export NVP_TREE_FILE="$BOOST_DIR"/bin/nvp_nvp.tree
                    if (( job == 1 )); then
                        BW=$(LD_PRELOAD=$BOOST_DIR/libnvp.so fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=randwrite -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    else
                        BW=$(LD_PRELOAD=$BOOST_DIR/libnvp.so fio -directory=/mnt/pmem0 -fallocate=none -direct=0 -iodepth 1 -thread -numjobs="$job" -rw=randwrite -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    fi
                elif [[ "${file_system}" == "MadFS" ]]; then
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
                    export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH
                    if (( job == 1 )); then
                        BW=$(LD_PRELOAD=$MADFS_DIR/build-release/libmadfs.so fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=randwrite -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    else
                        BW=$(LD_PRELOAD=$MADFS_DIR/build-release/libmadfs.so fio -directory=/mnt/pmem0 -fallocate=none -direct=0 -iodepth 1 -thread -numjobs="$job" -rw=randwrite -ioengine=sync -bs="4K" -size="$fsize"M -name=test | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                    fi
                else
                    bash "$TOOLS_PATH"/setup.sh "$file_system" "main" "0"
                    BW=$(bash "$TOOLS_PATH"/fio.sh "$fpath" 4K "$fsize" "$job" | grep WRITE: | awk '{print $2}' | sed 's/bw=//g' | "$TOOLS_PATH"/converter/to_MiB_s)
                fi
                table_add_row "$TABLE_NAME" "$file_system rnd-write $job $fsize $BW"
           done
        done
    done
done