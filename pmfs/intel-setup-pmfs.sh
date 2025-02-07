#!/bin/sh

sudo umount /mnt/pmem1
sudo rmmod pmfs
make
sudo insmod pmfs.ko measure_timing=0

# sleep 1

sudo mount -t pmfs -o init /dev/pmem1 /mnt/pmem1

#cp test1 /mnt/ramdisk/
#dd if=/dev/zero of=/mnt/ramdisk/test1 bs=1M count=1024 oflag=direct
