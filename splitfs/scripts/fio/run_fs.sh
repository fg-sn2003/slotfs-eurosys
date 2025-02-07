#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: sudo ./run_fs.sh <fs> <run_id>"
    exit 1
fi

set -x

workload=fio
fs=$1
run_id=$2
current_dir=$(pwd)
fio_dir=`readlink -f ../../fio`
workload_dir=$fio_dir/workloads
pmem_dir=/mnt/pmem0
boost_dir=`readlink -f ../../splitfs`
result_dir=`readlink -f ../../results`
fs_results=$result_dir/$fs/$workload

if [ "$fs" == "splitfs-posix" ]; then
    run_boost=1
elif [ "$fs" == "splitfs-sync" ]; then
    run_boost=1
elif [ "$fs" == "splitfs-strict" ]; then
    run_boost=1
else
    run_boost=0
fi

ulimit -c unlimited

echo Sleeping for 5 seconds ..
sleep 5

run_workload()
{

    echo ----------------------- FIO WORKLOAD ---------------------------

    mkdir -p $fs_results
    rm $fs_results/run$run_id

    rm -rf $pmem_dir/*

    date

    if [ $run_boost -eq 1 ]; then
        # $boost_dir/run_boost.sh -p $boost_dir -t nvp_nvp.tree 
        export LD_LIBRARY_PATH=$boost_dir
        export NVP_TREE_FILE=$boost_dir/bin/nvp_nvp.tree
        LD_PRELOAD=$boost_dir/libnvp.so fio --ioengine=sync -fsync=1 --name=test --bs=4k --readwrite=randwrite --size=1G --filename=$pmem_dir/testfile 2>&1 | tee $fs_results/run$run_id
    else
	    fio --ioengine=sync --name=test --bs=4k --readwrite=write --size=4G --filename=$pmem_dir/testfile 2>&1 | tee $fs_results/run$run_id
    fi

    date

    cd $current_dir

    echo Sleeping for 5 seconds . .
    sleep 5

}

run_workload

cd $current_dir
