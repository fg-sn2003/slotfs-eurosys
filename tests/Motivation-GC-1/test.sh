#!/usr/bin/bash

source "../common.sh"
ABS_PATH=$(where_is_script "$0")
TOOLS_PATH=$ABS_PATH/../../tools

SIZES=("1G" "2G" "4G" "8G" "16G" "32G" "64G" "128G")
FSIZES=("256M" "512M" "1G" "2G" "4G" "8G" "16G" "32G")

PATTERNS=("sw" "rw" "sow" "row")

output_file="performance-table"
echo > "$output_file"

mkdir -p DATA
dmesg -c

for pattern in "${PATTERNS[@]}"; do
    mkdir -p DATA/$pattern
    for i in "${!SIZES[@]}"; do
        size="${SIZES[$i]}"
        fsize="${FSIZES[$i]}"

        bash "$TOOLS_PATH"/setup.sh "NOVA" "null" "0"
        echo "$pattern with size: $size"

        if [ $pattern == "sw" ]; then
            fio_output=$(sudo fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw="write" -ioengine=sync -bs="4K" -size=$size -name=test)
        elif [ $pattern == "rw" ] ; then
            fio_output=$(sudo fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw="randwrite" -ioengine=sync -bs="4K" -size=$size -name=test)
        elif [ $pattern == "sow" ]; then
            fio_output=$(sudo fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw="write" -ioengine=sync -bs="4K" -size=$size -filesize=$fsize -overwrite=1 -name=test)
        elif [ $pattern == "row" ]; then
            fio_output=$(sudo fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw="randwrite" -ioengine=sync -bs="4K" -size=$size -filesize=$fsize -overwrite=1 -name=test)
        fi

        dmesg_file="DATA/$pattern/dmesg_output_${size}"
        fio_file="DATA/$pattern/fio_output_${size}"

        echo $fio_output > "$fio_file"
        umount /mnt/pmem0
        dmesg -c > "$dmesg_file"
        
        bandwidth=$(grep -oP 'BW=\K[0-9]+' "$fio_file")
        fast_gc_time=$(grep -oP 'nova: log_fast_gc: count \d+, timing \K\d+' "$dmesg_file" | head -1)
        thorough_gc_time=$(grep -oP 'nova: log_thorough_gc: count \d+, timing \K\d+' "$dmesg_file" | head -1)
        cow_write_time=$(grep -oP 'nova: do_cow_write: count \d+, timing \K\d+' "$dmesg_file" | head -1)
        
        bandwidth=${bandwidth:-0}
        fast_gc_time=${fast_gc_time:-0}
        thorough_gc_time=${thorough_gc_time:-0}
        cow_write_time=${cow_write_time:-0}
        
        echo "$pattern $size $bandwidth $fast_gc_time $thorough_gc_time $cow_write_time" >> "$output_file"
    done
done