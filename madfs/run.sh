#!/usr/bin/env bash

LD_PRELOAD=./build-release/libmadfs.so /usr/local/filebench/filebench -f ./fileserver

LD_PRELOAD=./build-debug/libmadfs.so fio -filename="/mnt/pmem0/test" -fallocate=none -direct=0 -iodepth 1 -rw=write -ioengine=sync -bs="4K" -size=1024M -name=test