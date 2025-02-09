source "../common.sh"
ABS_PATH=$(where_is_script "$0")
TOOLS_PATH=$ABS_PATH/../../tools

rm -rf performance-table
mkdir -p performance-table

FILE_SYSTEMS=("NOVA" "NOVA-nogc" "NOVA-fragment")
PATTERNS=("sw" "rw" "sow" "row")

size="128G"
filesize="32G"

loop=1
if [ "$1" ]; then
    loop=$1
fi

for ((i = 1; i <= loop; i++)); do
    for file_system in "${FILE_SYSTEMS[@]}"; do
        for pattern in "${PATTERNS[@]}"; do
            mkdir -p performance-table/"$file_system"
            output_file="performance-table/$file_system/"$pattern"_$i"
            bash "$TOOLS_PATH"/setup.sh "$file_system" "null" "0"
            if [ $pattern == "sw" ]; then
                sudo fio -filename=/mnt/pmem0/test -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs=4K -size=$size -name=test -write_bw_log=$output_file -log_avg_msec=500 --runtime=180
            elif [ $pattern == "rw" ]; then
                sudo fio -filename=/mnt/pmem0/test -fallocate=none -direct=0 -iodepth 1 -rw=randwrite -ioengine=sync -bs=4K -size=$size -name=test -write_bw_log=$output_file  -log_avg_msec=500 --runtime=180
            elif [ $pattern == "sow" ]; then
                sudo fio -filename=/mnt/pmem0/test -fallocate=none -direct=0 -iodepth 1 -rw=write -overwrite=1 -ioengine=sync -bs=4K -size=$size -filesize=$filesize -name=test -write_bw_log=$output_file -log_avg_msec=500 --runtime=180
            elif [ $pattern == "row" ]; then
                sudo fio -filename=/mnt/pmem0/test -fallocate=none -direct=0 -iodepth 1 -rw=randwrite -overwrite=1 -ioengine=sync -bs=4K -size=$size -filesize=$filesize -name=test -write_bw_log=$output_file -log_avg_msec=500 --runtime=180
            fi            
        done
    done
done
