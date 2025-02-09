SIZES=("1G" "2G" "4G" "8G" "16G" "32G" "64G" "128G")
FSIZES=("256M" "512M" "1G" "2G" "4G" "8G" "16G" "32G")
PATTERNS=("sw" "rw" "sow" "row")

nova_path="/home/zyf/nvmfs/nova"
mkdir -p DATA
dmesg -c

for pattern in "${PATTERNS[@]}"; do
    mkdir -p DATA/$pattern
    for i in "${!SIZES[@]}"; do
        size="${SIZES[$i]}"
        fsize="${FSIZES[$i]}"
        
        bash $nova_path/setup.sh
        echo "$pattern with size: $size  fsize: $fsize"

        if [ $pattern == "sw" ]; then
            fio_output=$(sudo fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw="write" -ioengine=sync -bs="4K" -size=$size -name=test)
        elif [ $pattern == "rw" ] ; then
            fio_output=$(sudo fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw="randwrite" -ioengine=sync -bs="4K" -size=$size -name=test)
        elif [ $pattern == "sow" ]; then
            fio_output=$(sudo fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw="write" -ioengine=sync -bs="4K" -size=$size -filesize=$fsize -overwrite=1 -name=test)
        elif [ $pattern == "row" ]; then
            fio_output=$(sudo fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw="randwrite" -ioengine=sync -bs="4K" -size=$size -filesize=$fsize -overwrite=1 -name=test)
        fi

        echo $fio_output > DATA/$pattern/"fio_output_$size"
        umount /mnt/pmem0
        dmesg -c > DATA/$pattern/"dmesg_output_$size"
    done
done 

