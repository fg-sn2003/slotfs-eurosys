#!/usr/bin/env bash

make BUILD_TARGETS="madfs"

sudo umount /mnt/pmem0
sudo mkfs.ext4 -F -b 4096 /dev/pmem0
sudo mount -o dax /dev/pmem0 /mnt/pmem0
export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH
LD_PRELOAD=./build-release/libmadfs.so /usr/local/filebench/filebench -f ./fileserver.f