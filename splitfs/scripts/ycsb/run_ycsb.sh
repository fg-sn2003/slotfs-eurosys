#!/bin/bash

src_dir=`readlink -f ../../`
cur_dir=`readlink -f ./`
setup_dir=`readlink -f ../configs`
pmem_dir=/mnt/pmem0

run_ycsb()
{
    fs=$1
    for run in 1
    do
        sudo rm -rf $pmem_dir/*
        sudo taskset -c 0-7 ./run_fs.sh LoadA $fs $run
        sleep 5
        sudo taskset -c 0-7 ./run_fs.sh RunA $fs $run
        sleep 5
        sudo taskset -c 0-7 ./run_fs.sh RunB $fs $run
        sleep 5
        sudo taskset -c 0-7 ./run_fs.sh RunC $fs $run
        sleep 5
        sudo taskset -c 0-7 ./run_fs.sh RunF $fs $run
        sleep 5
        sudo taskset -c 0-7 ./run_fs.sh RunD $fs $run
        sleep 5
        sudo taskset -c 0-7 ./run_fs.sh LoadE $fs $run
        sleep 5
        sudo taskset -c 0-7 ./run_fs.sh RunE $fs $run
        sleep 5
    done
}

sudo $setup_dir/dax_config.sh
run_ycsb dax

sudo $setup_dir/nova_relaxed_config.sh
run_ycsb relaxed_nova

sudo $setup_dir/pmfs_config.sh
run_ycsb pmfs

sudo $setup_dir/nova_config.sh
run_ycsb nova

cd $setup_dir
sudo $setup_dir/nova_config.sh
cd $current_dir

export LEDGER_DATAJ=0
export LEDGER_POSIX=1
export LEDGER_YCSB=1
sudo $setup_dir/dax_config.sh
run_ycsb posix_boost
