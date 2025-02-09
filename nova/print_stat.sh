#!/usr/bin/bash

# set -e

function test_nova() {
    sudo bash setup.sh config.mt.json
}

function test_ext4_dax() {
    sudo umount /mnt/pmem
    sudo mkfs.ext4 /dev/pmem1
    sudo mount -t ext4 -o dax /dev/pmem1 /mnt/pmem
}

test_nova

# sudo fio -filename=/mnt/pmem/test1 -fallocate=none -direct=1 -iodepth 1 -rw=write -ioengine=sync -bs=4K -thread -numjobs=1 -size=100G -name=randrw # --dedupe_percentage=80 -group_reporting
sudo fio -filename=/mnt/pmem1/test1 -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs=4M -thread -numjobs=1 -size=1G -name=randrw # --dedupe_percentage=80 -group_reporting

sudo gcc ioctl_test.c -o ioctl_test && sudo ./ioctl_test
#sudo gcc ioctl_table_stat.c -o ioctl_table_stat && sudo ./ioctl_table_stat

#time sudo rm /mnt/pmem/test1

#sudo gcc ioctl_test.c -o ioctl_test && sudo ./ioctl_test
#sudo dmesg | tail -n 50

#sudo umount /mnt/pmem
#sudo rmmod nova
sudo dmesg | tail -n 50

